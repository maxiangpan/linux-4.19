#include<stdio.h>
#include <stdlib.h>
void Imgcvt(unsigned char *in,unsigned char *out)
{
    int height = 1300;
    int width = 1600;
    int index = 0;
    for(int y =0;y< height ;y++) {
        //printf("index int = 0x%x\n",in[index]);
        int line_posy = y * width *2;
        for(int x=0;x<width *2;x+=2){
            out[index] = ((in[line_posy + x])>>2)+(((in[line_posy + x + 1])&0x03)<<6);
            index++;
        }
    }
}

void convertToYUVY(unsigned char *in, unsigned char *out, int width, int height) {
    int index_in = 0;
    int index_out = 0;
    unsigned int yuv = 0x00000000;
    unsigned int yuv_ptr = NULL;

    for (int y = 0; y < height; y++) {
        int line_pos = y * width * 2;
        for (int x = 0; x < width*2; x += 4) {
            unsigned char Y1 = ((in[line_pos + x])>>2)+(((in[line_pos + x + 1])&0x03)<<6);        // 第一个像素的Y值
            unsigned char Y2 = ((in[line_pos + x])>>2)+(((in[line_pos + x + 1])&0x03)<<6);    // 第二个像素的Y值

            unsigned char U = 0x80;
            unsigned char V = 0x80;

            Y1 = (unsigned char)((Y1 & 0xFFFF0000)>>16>>2);
            Y2 = (unsigned char)((Y2 & 0x0000fFFF)>>2);

            yuv_ptr = (unsigned int *)(in + 4 *x*y);
            yuv = *(unsigned int *)(in + 4 *x*y);
            // *yuv_ptr = (U << 24) | (Y2 << 16) | (V << 8) | Y1;
            // unsigned int temp = *(unsigned int *)(in + index_in + x);
            // *(unsigned int *)(in + index_in + x) = yuv;
            // *(unsigned int *)(in + index_in + x + 4) = temp;
            // out[index_out + 1] = U;
            // out[index_out + 2] = Y1; 
            // out[index_out + 3] = V;
            // out[index_out + 4] = Y2; 
            //index_in += 4;
            // index_out += 4;
        }
        //index_in += width * 2;
    }
    in = yuv;
}

int main(int argc, char *argv[])
{
    int i=0;
    int j=0;
    unsigned char *ptr = NULL;
    unsigned char *yuv_ptr_y = NULL;
    unsigned char *yuv_ptr = NULL;

    ptr = (unsigned char *)malloc(1600*1300*2);

    ptr = (unsigned char *)malloc(1600*1300*2);

    yuv_ptr_y = (unsigned char *)malloc(1600*1300*2);
    yuv_ptr = (unsigned char *)malloc(1600*1300*4);
    printf("mallc ptr = %p\n",ptr);
    FILE *fp = fopen("123.raw","rb");
    fread(ptr,1600*1300 *2,1,fp);
    fclose(fp);

    // fp = fopen("test_0.raw","wb");
    // fwrite(ptr,1600*1300 *4,1,fp);

    printf("readd data from file\n");
    Imgcvt(ptr,yuv_ptr_y);

    fp = fopen("test1.raw","wb");
    fwrite(yuv_ptr_y,1600*1300,2,fp);

    convertToYUVY(ptr,yuv_ptr,1600,1300);

    // unsigned char Y1 = 0X00;
    // unsigned char Y2 = 0X00;
    // unsigned char U = 0X80;
    // unsigned char V = 0X80;

    // unsigned char yuv = 0x00000000;
    // unsigned char *yuv_ptr = NULL;
    
    // for(i=0; i<1600*1300*2/4; i++)
    // {
    //     yuv_ptr = (unsigned char *)(ptr + 4 *i);
    //     yuv = *(unsigned char *)(ptr + 4 *i);
    //     Y2 = (unsigned char)((yuv & 0xFFFF0000 )>> 16 >> 2);
    //     Y1 = (unsigned char)((yuv & 0x0000FFFF )>> 2);
    //     *yuv_ptr = (U << 24) | (Y2 << 16) | (V << 8) | Y1;
    // }


    fp = fopen("test3.raw","wb");
    fwrite(ptr,1600*1300*2,1,fp);
    fclose(fp);
    free(ptr);
    return 0;
}