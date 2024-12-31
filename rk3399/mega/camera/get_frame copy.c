#include <stdio.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <time.h>
#include "get_frame.h"

#define MAX_FILENAME_LENGTH 256
#define TEXT_COLOR_Y 255   // 使用白色字体
#define TEXT_COLOR_U 128   // 使用适中的色度值
#define TEXT_COLOR_V 128   // 使用适中的色度值
#define FONT_WIDTH 32 // 增大字体宽度
#define FONT_HEIGHT 32 // 增大字体高度

void save_frame(const char *filename, uint8_t *data, size_t num_bytes) {
    static int counter = 1; // 用于计数
    char new_filename[MAX_FILENAME_LENGTH];

    // 构建新的文件名
    snprintf(new_filename, MAX_FILENAME_LENGTH, "%s_%d.raw", filename, counter);

    FILE *file = fopen(new_filename, "wb");
    if (file) {
        fwrite(data, sizeof(uint8_t), num_bytes, file);
        fclose(file);
        counter++; // 增加计数器
    } else {
        perror("Failed to open file");
    }
}

#ifdef SDL_DISPLAY
// 创建 SDL 窗口和渲染器
int init_sdl(int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow("Video Frame", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture) {
        fprintf(stderr, "Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

// 将 YUV 数据渲染到 SDL 窗口
void display_frame(SDL_Renderer *renderer, SDL_Texture *texture, uint8_t *yuv_data, int width, int height) {
    SDL_UpdateTexture(texture, NULL, yuv_data, width); // YUV422每个像素占用2字节
    SDL_RenderClear(renderer); // 清空渲染目标
    SDL_RenderCopy(renderer, texture, NULL, NULL); // 复制纹理到渲染目标
    SDL_RenderPresent(renderer); // 更新屏幕显示
}

void close_sdl() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
#else
// 创建 X11 窗口
int init_x11(int width, int height) {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open X display\n");
        return -1;
    }

    int screen = DefaultScreen(display);
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, width, height, 1, BlackPixel(display, screen), WhitePixel(display, screen));
    XMapWindow(display, window);

    gc = XCreateGC(display, window, 0, NULL);
    XFlush(display);

    // 创建一个 XImage 对象用于显示 YUV 图像
    ximage = XCreateImage(display, DefaultVisual(display, screen), 24, ZPixmap, 0, (char*)malloc(width * height * 4), width, height, 32, 0);
    return 0;
}

// 将 YUV 数据转换为 RGB 格式并刷新到 X11 窗口
void display_frame(uint8_t *yuv_data, int width, int height) {
    // 将 YUV 数据转换为 RGB 格式（简单的转换）
    for (int i = 0; i < width * height; i++) {
        int y = yuv_data[i];
        int u = yuv_data[width * height + (i / 2)];
        int v = yuv_data[width * height + (i / 2) + 1];

        int r = y + 1.402 * (v - 128);
        int g = y - 0.344136 * (u - 128) - 0.714136 * (v - 128);
        int b = y + 1.772 * (u - 128);

        // 限制 RGB 范围
        r = (r > 255) ? 255 : (r < 0) ? 0 : r;
        g = (g > 255) ? 255 : (g < 0) ? 0 : g;
        b = (b > 255) ? 255 : (b < 0) ? 0 : b;

        ximage->data[i * 4] = r;
        ximage->data[i * 4 + 1] = g;
        ximage->data[i * 4 + 2] = b;
        ximage->data[i * 4 + 3] = 255; // Alpha 通道
    }

    // 更新 X11 窗口
    XPutImage(display, window, gc, ximage, 0, 0, 0, 0, width, height);
    XFlush(display);
}

void close_x11() {
    if (ximage) {
        free(ximage->data);
        XDestroyImage(ximage);
    }
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}
#endif

// 绘制字符到 YUV 图像上
void draw_text_on_yuv(uint8_t *yuv_data, int width, int height, const char *text, int x, int y) {
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c < 32 || c > 127) continue; // 跳过非可打印字符

        const uint8_t *bitmap = FONT[c - 32]; // 获取字符位图数据
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (bitmap[row] & (1 << (7 - col))) { // 检查位图中该位置是否为 1
                    int px = x + i * 8 + col;
                    int py = y + row;
                    if (px < width && py < height) {
                        yuv_data[py * width + px] = TEXT_COLOR_Y; // 设置亮度
                        if (py % 2 == 0 && px % 2 == 0) {
                            int u_offset = (py / 2) * (width / 2) + (px / 2);
                            int v_offset = u_offset;
                            yuv_data[width * height + u_offset] = TEXT_COLOR_U; // 设置色度U
                            yuv_data[width * height + v_offset] = TEXT_COLOR_V; // 设置色度V
                        }
                    }
                }
            }
        }
    }
}

// 添加时间戳水印
void add_timestamp_watermark(AVFrame *frame, int width, int height) {
    time_t raw_time;
    struct tm *time_info;
    char time_string[80];

    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);
    // printf("Adding timestamp: %s\n", time_string);

    // 在右上角绘制时间戳
    draw_text_on_yuv(frame->data[0], width, height, time_string, width - (strlen(time_string) * FONT_WIDTH), 0);
    // int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_YUV422P, width, height, 1);
    // save_frame("output_frame", frame->data[0], num_bytes);
}

int main(int argc, char *argv[]) {
    // 初始化 FFmpeg 库
    avformat_network_init();

    // 输入文件路径
    const char *input_filename = "../2.mp4";
    
    //打开文件
    AVFormatContext *fmt_ctx = NULL;
    if (avformat_open_input(&fmt_ctx, input_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", input_filename);
        return -1;
    }

    // 查找流信息
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return -1;
    }

    // 查找视频流
    int video_stream_index = -1;
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        fprintf(stderr, "Could not find video stream\n");
        return -1;
    }

    // 获取解码器
    AVCodecContext *codec_ctx = avcodec_alloc_context3(NULL);
    if (!codec_ctx) {
        fprintf(stderr, "Could not allocate codec context\n");
        return -1;
    }

    avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[video_stream_index]->codecpar);
    AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        return -1;
    }

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return -1;
    }

    int width = codec_ctx->width;
    int height = codec_ctx->height;

#ifdef SDL_DISPLAY
    if (init_sdl(width, height) < 0) {
        return -1;
    }
#endif
    // 读取视频帧
    AVPacket packet;
    int frame_count = 0;
    AVFrame *frame = av_frame_alloc();
    AVFrame *frame_yuyv = av_frame_alloc();
    uint8_t *buffer = NULL;

    struct SwsContext *sws_ctx = NULL;
    int64_t last_pts = 0; // 上一帧的时间戳

    AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;
    double fps = av_q2d(fmt_ctx->streams[video_stream_index]->avg_frame_rate);
    double delay = 1.0 / fps; // 每帧的延迟时间（秒）

    while (av_read_frame(fmt_ctx, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            // 解码视频帧
            int ret = avcodec_send_packet(codec_ctx, &packet);
            if (ret < 0) {
                fprintf(stderr, "Error sending packet to decoder\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "Error receiving frame from decoder\n");
                    break;
                }

                // 计算当前帧的时间戳（单位：微秒）
                int64_t pts = frame->best_effort_timestamp;
                if (pts == AV_NOPTS_VALUE) {
                    pts = frame_count * delay * AV_TIME_BASE; // 如果没有时间戳，手动设置
                }

                // 计算帧的播放时间（以微秒为单位）
                int64_t target_time = av_rescale_q(pts, time_base, AV_TIME_BASE_Q); // 将时间戳转换为微秒

                // 获取当前时间（微秒）
                int64_t current_time = av_gettime();

                // 比较当前时间与目标时间，若需要延迟，则进行处理
                if (current_time < target_time) {
                    int64_t delay_time = target_time - current_time;
                    // printf("Delaying for %ld microseconds\n", delay_time);
                    usleep(delay_time/100000); // 延迟到目标时间
                } else {
                    // 如果当前时间已经超过了目标时间（可能由于帧率不稳定），跳过帧
                    printf("Skipping frame, current time %ld, target time %ld\n", current_time, target_time);
                }

                // 选择第一帧进行处理
                // if (frame_count == 10) 
                if (1) 
                {
                    // 设置 YUYV frame 大小
                    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);
                    buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
                    av_image_fill_arrays(frame_yuyv->data, frame_yuyv->linesize, buffer, AV_PIX_FMT_YUV420P, width, height, 1);

                    // 将解码的帧转换为 YUYV 格式
                    sws_ctx = sws_getContext(width, height, codec_ctx->pix_fmt, width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
                    sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0, height, frame_yuyv->data, frame_yuyv->linesize);

                    add_timestamp_watermark(frame_yuyv, width, height);
                    // printf("Frame %d saved\n", frame_count);
                    // save_frame("output_frame", frame_yuyv->data[0], num_bytes);

                    // 显示当前帧
#ifdef SDL_DISPLAY
                    display_frame(renderer, texture, frame_yuyv->data[0], width , height);
#else
                    display_frame(frame_yuyv->data[0], width, height);
#endif
                }
                frame_count++;

                // if(frame_count > 10){
                //     printf("Frame count: %d\n", frame_count);
                //     goto clean;
                // }
            }
        }

        av_packet_unref(&packet);
    }
clean :
    // 清理资源
    close_sdl();
    av_frame_free(&frame);
    av_frame_free(&frame_yuyv);
    avcodec_free_context(&codec_ctx);
    sws_freeContext(sws_ctx);
    av_packet_unref(&packet);
    av_free(buffer);
    avformat_close_input(&fmt_ctx);

    return 0;
}
