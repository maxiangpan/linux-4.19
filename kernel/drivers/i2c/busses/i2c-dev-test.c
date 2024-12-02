/*
 *  i2c-versatile.c
 *
 *  Copyright (C) 2006 ARM Ltd.
 *  written by Russell King, Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>//内核头文件/*包含printk等操作函数*/
#include <linux/module.h>//模块加载卸载函数
#include <linux/types.h>//数据类型定义
#include <linux/cdev.h>/*cdev_init cdev_add等函数*/
#include <linux/device.h>//class_create等函数
#include <linux/uaccess.h>
#include <linux/i2c.h> /*i2c相关api*/
#include <linux/i2c-algo-bit.h>
#include <linux/init.h>
#include <linux/platform_device.h>/*platform device*/
#include <linux/slab.h> /*kmalloc、kfree函数*/
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/delay.h> /*内核延时函数*/
#include <linux/gpio.h>/*gpio接口函数*/
#include <linux/of.h>/*设备树操作相关的函数*/
#include <linux/fs.h>//file_operations结构体
#include <asm/gpio.h>/*gpio接口函数*/
#include <asm/uaccess.h>/*__copy_from_user 接口函数*/

#define I2C_DEVICE_NAME "i2c-3"
#define I2C_DEVICE_SIZE  256  /*24c02 为256字节*/
#define I2C_DEVICE_COUNT 1

#define I2C_CONTROL	0x00
#define I2C_CONTROLS	0x00
#define I2C_CONTROLC	0x04
#define SCL		(1 << 0)
#define SDA		(1 << 1)

struct i2c_dev {
	struct i2c_adapter	 adap;
	struct i2c_algo_bit_data algo;
	void __iomem		 *base;

	struct gpio_desc *sda;
	struct gpio_desc *scl;
	struct i2c_client	*client; /*适配器 probe函数中会填充此变量*/

	dev_t dev_num;
	struct cdev cdev;
	struct class *i2c_class;
	struct device *i2c_device;

	int flag;

	/*使用SMBUS协议方式读写*/
	int use_smbus;
	int use_smbus_write;

	struct mutex lock;
};

static unsigned char io_limit = 128; /*一次最多读取128字节*/
static unsigned char write_timeout = 25;/*i2c通信超时时间*/
static unsigned char at24cxx_page_size = 8;/*at24c02 每页8字节*/

static ssize_t i2c_dev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t i2c_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static int i2c_dev_open(struct inode *inode, struct file *file);
static int i2c_dev_release(struct inode *inode, struct file *file);
static long i2c_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = i2c_dev_open,
    .release = i2c_dev_release,
    .read = i2c_dev_read,
    .write = i2c_dev_write,
	.unlocked_ioctl = i2c_dev_ioctl,
};

static ssize_t at24_eeprom_read(struct i2c_dev *at24, char *buf,unsigned offset, size_t count)
{
	struct i2c_msg msg[2];
	u8 msgbuf[2];
	struct i2c_client *client;
	unsigned long timeout, read_time;
	int status, i;
    
	memset(msg, 0, sizeof(msg));

   /*获取设备信息*/
	client = at24->client;

	if (count > io_limit)
		count = io_limit;

	if (at24->use_smbus) 
   {
		/* Smaller eeproms can work given some SMBus extension calls */
		if (count > I2C_SMBUS_BLOCK_MAX)
			count = I2C_SMBUS_BLOCK_MAX;
	} 
   else 
   {

		i = 0;
		msgbuf[i++] = offset;/*读取首地址*/

      /* msg[0]为发送要读取的首地址 */
		msg[0].addr = client->addr;
		msg[0].buf = msgbuf;
		msg[0].len = i;

      /* msg[1]读取数据 */
		msg[1].addr = client->addr;
		msg[1].flags = I2C_M_RD;
		msg[1].buf = buf;        /* 读取数据缓冲区*/
		msg[1].len = count;      /* 读取数据长度 */
	}
   
   /*超时时间设置为write_timeout毫秒*/
	timeout = jiffies + msecs_to_jiffies(write_timeout);
	do {
		read_time = jiffies;
		if (at24->use_smbus) /*使用SMBUS协议*/
		{
			status = i2c_smbus_read_i2c_block_data_or_emulated(client, offset,count, buf);
		} 
		else /*普通I2C协议*/
		{
			status = i2c_transfer(client->adapter, msg, 2);
			if (status == 2)
				status = count;
		}
			printk("read %zu@%d --> %d (%ld)\n",count, offset, status, jiffies);

		if (status == count)
			return count;
	
		/* REVISIT: at HZ=100, this is sloooow */
		msleep(1);
	} while (time_before(read_time, timeout));   
	return -ETIMEDOUT;
}

static ssize_t at24_eeprom_write(struct i2c_dev *at24,  char *buf,unsigned offset, size_t count)
{
	struct i2c_client *client;
	struct i2c_msg msg;
	ssize_t status = 0;
	unsigned long timeout, write_time;
    int i = 0;
	
   /*获取设备信息*/
	client = at24->client;

	/* 最大写入数据是1页 */
	if (count > at24cxx_page_size)
		count = at24cxx_page_size;

    /*msg.buf申请内存*/
    msg.buf = kmalloc(count+2,GFP_KERNEL);
    if(!msg.buf)
	   return -ENOMEM;
    
	/* 不使用SMBUS协议  需要填充msg */
	if (!at24->use_smbus) 
    {
		msg.addr = client->addr;
		msg.flags = 0;           /*标记为写数据*/

		msg.buf[i++] = offset;   /*写的起始地址*/
		memcpy(&msg.buf[i], buf, count);
		msg.len = i + count;     /*写数据的长度*/
	}


	timeout = jiffies + msecs_to_jiffies(write_timeout);
	do {
		write_time = jiffies;
		if (at24->use_smbus_write) 
		{
			switch (at24->use_smbus_write) 
			{
				case I2C_SMBUS_I2C_BLOCK_DATA:
				status = i2c_smbus_write_i2c_block_data(client,offset, count, buf);break;
				case I2C_SMBUS_BYTE_DATA:
				status = i2c_smbus_write_byte_data(client,offset, buf[0]);break;
			}
			if (status == 0)
				status = count;
		} 
		else 
		{
				status = i2c_transfer(client->adapter, &msg, 1);
				if (status == 1)
					status = count;
		}
		printk("write %zu@%d --> %zd (%ld)\n",count, offset, status, jiffies);
		if (status == count)
		{
			kfree(msg.buf);
			return count;
		}				
		/* REVISIT: at HZ=100, this is sloooow */
		msleep(1);
	} while (time_before(write_time, timeout));

    kfree(msg.buf);
	return -ETIMEDOUT;
}

static ssize_t i2c_dev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    // 实现读取I2C数据的代码
	printk("ready i2c_dev_read");
int ret = -EINVAL;
	char *buffer;/*数据缓存区*/
	unsigned char pos = file->f_pos; /*读取位置*/

	struct i2c_dev *i2c = file->private_data;  
	printk("r_count = %d  r_pos = %d\n",count,pos);

	buffer =(char *)kmalloc(count,GFP_KERNEL);
	if(!buffer)
		return -ENOMEM;

	mutex_lock(&i2c->lock);

	ret = at24_eeprom_read(i2c,buffer,pos,count);
	if(ret < 0 )
	{
		printk("at24c02 read error\n");
		kfree(buffer);
		return ret;
	}

	/*将读取到的数据返回用户层*/
	ret = copy_to_user(buf,(void *)buffer,ret);

	mutex_unlock(&i2c->lock);

	/*释放缓冲区内存*/
	kfree(buffer);
	return 0;
}

static ssize_t i2c_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    int ret = -EINVAL;
	char *buffer;/*缓冲区*/
	unsigned char pages;/*页数*/
	unsigned char num;/*不足一页剩下的字节数*/
	unsigned char pos = file->f_pos;/*写入的地址*/
	int i = 0; 
	struct i2c_dev *i2c = file->private_data;

	printk("w_count = %d w_pos = %d\n",count,pos);

	buffer =(char *)kmalloc(count,GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	pages = count / at24cxx_page_size;
	num = count % at24cxx_page_size;

	/*将需要写入的数据拷贝到内核空间的缓冲区*/
	ret = copy_from_user((void *)buffer,buf,count);

	mutex_lock(&i2c->lock);

	for(i = 0; i < pages; i++)
	{
        ret = at24_eeprom_write(i2c,&buffer[i*at24cxx_page_size],pos,at24cxx_page_size);
		if(ret < 0) 
		{
			printk("at24c02 write error\n");
			kfree(buffer);
			return ret;
		}
		pos += 8;
	}

	if(num)
	{	
		ret = at24_eeprom_write(i2c,&buffer[i*at24cxx_page_size],pos,num);
		if(ret < 0) 
		{
			printk("at24c02 write error\n");
			kfree(buffer);
			return ret;
		}
	}

	mutex_unlock(&i2c->lock);	
   	/*释放缓冲区内存*/
   	kfree(buffer);
  	// devm_kfree(&dev->client->dev,buffer);
	return 0;
}

static int i2c_dev_open(struct inode *inode, struct file *file) {
    // 打开设备时的处理
	printk("ready i2c_dev_open");
	struct i2c_dev *i2c = file->private_data;
	i2c->use_smbus = 0;
	i2c->use_smbus_write = 0; 
    return 0;
}

static int i2c_dev_release(struct inode *inode, struct file *file) {
	printk("ready i2c_dev_release");
    // 关闭设备时的处理
    return 0;
}

static long i2c_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct i2c_dev *i2c = file->private_data;
    // 实现 ioctl 操作
    return 0;
}

static void i2c_dev_setsda(void *data, int state)
{
	struct i2c_dev *i2c = data;

	writel(SDA, i2c->base + (state ? I2C_CONTROLS : I2C_CONTROLC));
}

static void i2c_dev_setscl(void *data, int state)
{
	struct i2c_dev *i2c = data;

	writel(SCL, i2c->base + (state ? I2C_CONTROLS : I2C_CONTROLC));
}

static int i2c_dev_getsda(void *data)
{
	struct i2c_dev *i2c = data;
	return !!(readl(i2c->base + I2C_CONTROL) & SDA);
}

static int i2c_dev_getscl(void *data)
{
	struct i2c_dev *i2c = data;
	return !!(readl(i2c->base + I2C_CONTROL) & SCL);
}

static u32 i2c_dev_functionality(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_BYTE | 
           I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA | 
           I2C_FUNC_SMBUS_WRITE_I2C_BLOCK | I2C_FUNC_SMBUS_READ_I2C_BLOCK;
}

static const struct i2c_algo_bit_data i2c_dev_algo = {
	.setsda	= i2c_dev_setsda,
	.setscl = i2c_dev_setscl,
	.getsda	= i2c_dev_getsda,
	.getscl = i2c_dev_getscl,
	.udelay	= 30,
	.timeout = HZ,
};

static int i2c_dev_probe(struct platform_device *dev)
{
	struct i2c_dev *i2c;
	struct resource *r;
	int ret;

	printk("i2c test test\n");
	i2c = devm_kzalloc(&dev->dev, sizeof(struct i2c_dev), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;

	ret = alloc_chrdev_region(&i2c->dev_num, 0, I2C_DEVICE_COUNT, I2C_DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to allocate device number\n");
        return ret;
    }

	cdev_init(&i2c->cdev, &fops);
    i2c->cdev.owner = THIS_MODULE;

	ret = cdev_add(&i2c->cdev, i2c->dev_num, I2C_DEVICE_COUNT);
    if (ret) {
        printk(KERN_ALERT "Failed to add cdev\n");
        unregister_chrdev_region(i2c->dev_num, I2C_DEVICE_COUNT);
        return ret;
    }

	i2c->i2c_class = class_create(THIS_MODULE, "i2c_class");
	if (IS_ERR(i2c->i2c_class)) {
        printk(KERN_ALERT "Failed to create class\n");
        return PTR_ERR(i2c->i2c_class);
    }

	i2c->i2c_device = device_create(i2c->i2c_class, NULL, i2c->dev_num, NULL, I2C_DEVICE_NAME);
    if (IS_ERR(i2c->i2c_device)) {
        printk(KERN_ALERT "Failed to create device\n");
        class_destroy(i2c->i2c_class);
        return PTR_ERR(i2c->i2c_device);
    }

#if 1
	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	i2c->base = devm_ioremap_resource(&dev->dev, r);
	if (IS_ERR(i2c->base)){
		printk(KERN_INFO "Failed to map I2C registers: %ld\n", PTR_ERR(i2c->base));
		return PTR_ERR(i2c->base);
	}

	writel(SCL | SDA, i2c->base + I2C_CONTROLS);
#endif

	i2c->adap.owner = THIS_MODULE;
	strlcpy(i2c->adap.name, "I2C-dev adapter", sizeof(i2c->adap.name));
	i2c->adap.algo_data = &i2c->algo;
	i2c->adap.dev.parent = &dev->dev;
	i2c->adap.dev.of_node = dev->dev.of_node;
	i2c->algo = i2c_dev_algo;
	i2c->algo.data = i2c;

	ret = i2c_bit_add_numbered_bus(&i2c->adap);
	if (ret < 0){
		printk(KERN_INFO "Failed to add I2C bus: %d\n", ret);
		return ret;
	}
	platform_set_drvdata(dev, i2c);

	printk(KERN_INFO "I2C device driver initialized\n");

	return 0;
}

static int i2c_dev_remove(struct platform_device *dev)
{
	struct i2c_dev *i2c = platform_get_drvdata(dev);

	device_destroy(i2c->i2c_class, MKDEV(0, 0));
	class_destroy(i2c->i2c_class);
	cdev_del(&i2c->cdev);
	unregister_chrdev_region(i2c->dev_num, I2C_DEVICE_COUNT);

	printk(KERN_INFO "I2C device driver exited\n");

	//i2c_del_adapter(&i2c->adap);
	return 0;
}

static const struct of_device_id i2c_dev_match[] = {
	{ .compatible = "arm,i2c-test", },
	{},
};
MODULE_DEVICE_TABLE(of, i2c_dev_match);

static struct platform_driver i2c_dev_driver = {
	.probe		= i2c_dev_probe,
	.remove		= i2c_dev_remove,
	.driver		= {
		.name	= "i2c-dev-test",
		.of_match_table = i2c_dev_match,
	},
};

static int __init i2c_dev_init(void)
{
	return platform_driver_register(&i2c_dev_driver);
}

static void __exit i2c_dev_exit(void)
{
	platform_driver_unregister(&i2c_dev_driver);
}

subsys_initcall(i2c_dev_init);
module_exit(i2c_dev_exit);

MODULE_DESCRIPTION("ARM Versatile I2C bus driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:versatile-i2c");
