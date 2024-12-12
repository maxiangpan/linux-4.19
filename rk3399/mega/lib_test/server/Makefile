# 编译器
CC = gcc

# 编译选项
CFLAGS = -fPIC -Wall

# 目标文件名
TARGET = libserver.so

# 源文件
SRCS = server.c

# 目标文件名
OBJS = $(SRCS:.c=.o)

# 默认目标
all: $(TARGET)

# 生成共享库
$(TARGET): $(SRCS)
	$(CC) -shared -o $@ $<
	mv $@ $(CURENT_DIR)/$(FOLDER_NAME)

# 清理生成的文件
clean:
	rm -f $(OBJS) $(TARGET)
