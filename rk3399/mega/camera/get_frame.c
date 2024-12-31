#include <stdio.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <time.h>
#include "get_frame.h"

/*
1.码率
2.进度条
3.音视频同步

*/

#define MAX_FILENAME_LENGTH 256
#define TEXT_COLOR_Y 255   // 使用白色字体
#define TEXT_COLOR_U 128   // 使用适中的色度值
#define TEXT_COLOR_V 128   // 使用适中的色度值
#define FONT_WIDTH 16 // 增大字体宽度
#define FONT_HEIGHT 32 // 增大字体高度

typedef struct {
    AVFrame *frame;
    int64_t pts;
} VideoFrame;

#define MAX_QUEUE_SIZE 5

// 线程同步和数据队列
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
VideoFrame frame_queue[MAX_QUEUE_SIZE];
int queue_head = 0, queue_tail = 0;
int queue_size = 0;

pthread_mutex_t process_mutex = PTHREAD_MUTEX_INITIALIZER; // 用于处理线程
pthread_cond_t process_cond = PTHREAD_COND_INITIALIZER;

int enqueue_frame(AVFrame *frame, int64_t pts) {
    pthread_mutex_lock(&queue_mutex);
    if (queue_size < MAX_QUEUE_SIZE) {
        frame_queue[queue_tail].frame = frame;
        frame_queue[queue_tail].pts = pts;
        queue_tail = (queue_tail + 1) % MAX_QUEUE_SIZE;
        queue_size++;
        pthread_cond_signal(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);
        return 1;
    }
    pthread_mutex_unlock(&queue_mutex);
    return 0;
}

VideoFrame dequeue_frame() {
    VideoFrame frame;
    pthread_mutex_lock(&queue_mutex);
    // fprintf(stderr,"pass queue_size = %d \n",queue_size);
    while (queue_size == 0) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }
    frame = frame_queue[queue_head];
    queue_head = (queue_head + 1) % MAX_QUEUE_SIZE;
    queue_size--;
    pthread_mutex_unlock(&queue_mutex);
    return frame;
}

// 解码线程
void* decode_thread(void* arg) {
    AVFormatContext *fmt_ctx = (AVFormatContext*)arg;
    AVPacket packet;
    struct SwsContext *sws_ctx = NULL;
    int video_stream_index = -1;
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        fprintf(stderr, "Could not find video stream\n");
        return NULL;
    }

    AVCodecContext *codec_ctx = avcodec_alloc_context3(NULL);
    if (!codec_ctx) {
        fprintf(stderr, "Could not allocate codec context\n");
        return NULL;
    }
    // 初始化编解码器上下文
    avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[video_stream_index]->codecpar);
    AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        return NULL;
    }
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return NULL;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        return NULL;
    }
    AVFrame *frame_yuyv = av_frame_alloc();

    width = codec_ctx->width;
    height = codec_ctx->height;
    // 设置 YUYV frame 大小
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);
    detect_doned = 1;
    buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
    av_image_fill_arrays(frame_yuyv->data, frame_yuyv->linesize, buffer, AV_PIX_FMT_YUV420P, width, height, 1);


    AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;
    double fps = av_q2d(fmt_ctx->streams[video_stream_index]->avg_frame_rate);
    double delay = 1.0 / fps; // 每帧的延迟时间（秒）
    int64_t current_time = 0;

    int64_t last_pts = AV_NOPTS_VALUE;
    
    while (av_read_frame(fmt_ctx, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
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
#if 1
                int frame_fps = (fmt_ctx->streams[video_stream_index]->avg_frame_rate.num)/(fmt_ctx->streams[video_stream_index]->avg_frame_rate.den);
                // 比较当前时间与目标时间，若需要延迟，则进行处理
                if (current_time < frame->best_effort_timestamp) {
                    int64_t delay_time = frame->best_effort_timestamp - current_time;
                    //  printf("Delaying for %ld microseconds , current_time = %ld elay_time/10 = %ld fps = %d \n", frame->best_effort_timestamp , current_time,delay_time/10,fmt_ctx->streams[video_stream_index]->avg_frame_rate.num/fmt_ctx->streams[video_stream_index]->avg_frame_rate.den);
                    //usleep(delay_time/33000); // 延迟到目标时间      
                    // printf("fps = %d \n",frame_fps);
                    current_time = frame->best_effort_timestamp;
                    if(play_status == SDL_EVENT_FAST_FORWARD);
                    else
                        usleep(delay_time/16);
                } else {
                    // 如果当前时间已经超过了目标时间（可能由于帧率不稳定），跳过帧
                    // printf("Skipping frame, current time %ld, target time %ld\n", current_time, target_time);
                }
#endif
                // 将解码的帧转换为 YUYV 格式
#if 1
                sws_ctx = sws_getContext(width, height, codec_ctx->pix_fmt, width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
                sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0, height, frame_yuyv->data, frame_yuyv->linesize);
#endif
                // save_frame("output_frame", frame_yuyv->data[0], num_bytes);

                // usleep(33000); // 每帧间隔25ms

                enqueue_frame(frame_yuyv, pts);
                last_pts = pts;
                if(status == STATE_QUITE)
                {
                    goto exit;
                }
            }
        }
        av_packet_unref(&packet);
    }

exit :
    av_frame_free(&frame);
    av_frame_free(&frame_yuyv);
    avcodec_free_context(&codec_ctx);
    sws_freeContext(sws_ctx);
    av_packet_unref(&packet);
    av_free(buffer);
    free(buffer);

    return NULL;
}

// 处理线程
void* process_thread(void* arg) {
    int count = 0;
    double current_time = 0;  // 当前显示的时间
    while(!detect_doned)
    {
        usleep(1000);
    }
    if (init_sdl(width, height) < 0) {
        perror("Failed to open file");
        return NULL;
    }
    while (1) {
        VideoFrame video_frame = dequeue_frame();

        printf("play_status = %d \n",play_status);

        switch (play_status) {
            case SDL_EVENT_PAUSE:
                // 如果是暂停状态，跳过处理
                printf("暂停播放\n");
                SDL_Delay(100);  // 延时以避免CPU过度占用
                break;
            case SDL_EVENT_PLAY:
                // 正常播放状态，处理解码帧
                add_timestamp_watermark(video_frame.frame, width, height);
                display_frame(renderer, texture, video_frame.frame->data[0], width, height);
                break;
            case SDL_EVENT_FAST_FORWARD:
                // 快进状态，处理解码帧
                printf("快进播放\n");
                add_timestamp_watermark(video_frame.frame, width, height);
                display_frame(renderer, texture, video_frame.frame->data[0], width, height);
                count++;
                break;
            case SDL_EVENT_REWIND:
                // 快退状态，增加每帧的等待时间
                usleep(50000);  // 快退时增加每帧的等待时间
                count--;
                break;
            case STATE_QUITE:
                // 退出状态，退出循环
                printf("退出播放\n");
                return NULL;
            default:
                break;
        }
#if 0
        if (play_status == SDL_EVENT_PAUSE) {
            // 如果是暂停状态，跳过处理
            SDL_Delay(100);  // 延时以避免CPU过度占用
            continue;
        }    

        if (video_frame.frame != NULL) {
            // 添加时间戳水印
            add_timestamp_watermark(video_frame.frame, width, height);

            // 处理完的帧可以保存或者显示
            // save_frame("output_frame", video_frame.frame->data[0], num_bytes);
            if (video_frame.frame != NULL) {
            // 显示帧
                display_frame(renderer, texture, video_frame.frame->data[0], width, height);
            }
        }
        count ++ ;
#endif
    }
    return NULL;
}

// 显示线程
void* display_thread(void* arg) {
    int key_down_s = 0, key_down_a = 0, key_down_d = 0;  // 用于记录键盘状态
    int is_playing = 0;
    const Uint8 *keystate = NULL;
    int key_flag = 0;
#if 1
    while (1) {
        while (SDL_PollEvent(&e)) {
            keystate = SDL_GetKeyboardState(NULL);  // 获取当前键盘状态
            if (e.type == SDL_QUIT) {
                    sem_post(&sem);
                    play_status = SDL_EVENT_QUITE;
                    status = STATE_QUITE;
                    return 0;
                }
#if 0
            if (e.type == SDL_KEYDOWN) {
                // 处理键盘事件
                if (e.key.keysym.sym == SDLK_s) {  // 暂停/播放
                    play_status = (play_status == SDL_EVENT_PLAY) ? SDL_EVENT_PAUSE : SDL_EVENT_PLAY;
                    key_down_s = 1;  // 设置按键已按下
                } else if (e.key.keysym.sym == SDLK_a) {  // 快进
                    play_status = SDL_EVENT_FAST_FORWARD;
                    playback_speed = 2.0;  // 快进倍速
                    key_down_a = 1;  // 设置按键已按下
                } else if (e.key.keysym.sym == SDLK_d) {  // 快退
                    play_status = SDL_EVENT_REWIND;
                    playback_speed = 0.5;  // 快退倍速
                    key_down_d = 1;  // 设置按键已按下
                }
            }
            if (e.type == SDL_KEYUP) {
                // 处理按键松开事件
                if (e.key.keysym.sym == SDLK_s) {  // 按键 s 松开时恢复播放状态
                    key_down_s = 0;  // 重置按键状态
                } else if (e.key.keysym.sym == SDLK_a) {  // 快进松开时恢复播放状态
                    key_down_a = 0;
                    play_status = SDL_EVENT_PLAY;  // 复位为播放状态
                    playback_speed = 1.0;  // 恢复正常播放速度
                } else if (e.key.keysym.sym == SDLK_d) {  // 快退松开时恢复播放状态
                    key_down_d = 0;
                    play_status = SDL_EVENT_PLAY;  // 复位为播放状态
                    playback_speed = 1.0;  // 恢复正常播放速度
                }
            }
#endif
            // 检测按键状态，按下时持续触发
            
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_s) {
                    play_status = (play_status == SDL_EVENT_PLAY) ? SDL_EVENT_PAUSE : SDL_EVENT_PLAY;
                    break;
                }
            }

            if (keystate[SDL_SCANCODE_D]) {  // 快进 
                // play_status = (play_status = SDL_EVENT_FAST_FORWARD) ? SDL_EVENT_FAST_FORWARD : SDL_EVENT_PLAY;
                play_status = SDL_EVENT_FAST_FORWARD;
                key_flag = 1;
            }
            else if (key_flag) {        // 快进松开时恢复播放状态
                play_status = SDL_EVENT_PLAY;
                key_flag = 0;
                
            }

            if (keystate[SDL_SCANCODE_A]) {  // 快退
                play_status = SDL_EVENT_REWIND;
            }

        }
        SDL_Delay(30);  // 可以稍微延迟以减少CPU占用
    }
#endif
    return NULL;
}

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

    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_PING,"0");

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

    SDL_PollEvent(NULL);
    
    return 0;
}

// 将 YUV 数据渲染到 SDL 窗口
void display_frame(SDL_Renderer *renderer, SDL_Texture *texture, uint8_t *yuv_data, int width, int height) {
    SDL_UpdateTexture(texture, NULL, yuv_data, width); // YUV422每个像素占用2字节
    SDL_RenderClear(renderer); // 清空渲染目标
    SDL_RenderCopy(renderer, texture, NULL, NULL); // 复制纹理到渲染目标
    SDL_RenderPresent(renderer); // 更新屏幕显示
    SDL_PollEvent(NULL);
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
    if (yuv_data == NULL || text == NULL) return;

    // 放大倍数
    int scale = FONT_WIDTH/8; // 将每个像素放大 2 倍

    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c < 32 || c > 127) continue; // 跳过非可打印字符

        const uint8_t *bitmap = FONT[c - 32]; // 获取字符位图数据

        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (bitmap[row] & (1 << (7 - col))) { // 检查位图中该位置是否为 1
                    int px = x + i * 16 + col * scale; // 放大后每列间距
                    int py = y + row * scale;          // 放大后每行间距

                    // 确保坐标不超出图像边界
                    if (px < width && py < height) {
                        yuv_data[py * width + px] = TEXT_COLOR_Y; // 设置亮度
                        // 放大后的像素点 (px, py) 对应的 U 和 V 通道
                        for (int dy = 0; dy < scale; dy++) {
                            for (int dx = 0; dx < scale; dx++) {
                                int expanded_px = px + dx;
                                int expanded_py = py + dy;
                                if (expanded_px < width && expanded_py < height) {
                                    yuv_data[expanded_py * width + expanded_px] = TEXT_COLOR_Y; // 亮度
                                    if (expanded_py % 2 == 0 && expanded_px % 2 == 0) {
                                        int u_offset = (expanded_py / 2) * (width / 2) + (expanded_px / 2);
                                        int v_offset = u_offset;
                                        yuv_data[width * height + u_offset] = TEXT_COLOR_U; // 色度U
                                        yuv_data[width * height + v_offset] = TEXT_COLOR_V; // 色度V
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// 计算并返回当前的帧率
float calculate_fps() {
    time_t current_time;
    time(&current_time);

    // 如果当前时间和上次更新时间差超过1秒，则更新帧率
    if (current_time - last_time >= 1) {
        fps = fps_frame_count;        // 当前秒的帧数即为帧率
        fps_frame_count = 0;          // 重置帧计数器
        last_time = current_time; // 更新上次计算时间
    }
    // fprintf(stderr, "fps = %d,frame = %d \n", fps,fps_frame_count);
    fps_frame_count++; // 每次调用增加一帧
    return fps;
}
// 添加时间戳水印
void add_timestamp_watermark(AVFrame *frame, int width, int height) {
    time_t raw_time;
    struct tm *time_info;
    char time_string[80];
    char fps_string[80];
    char watermark_text[160];

    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);
    // printf("Adding timestamp: %s\n", time_string);

    float current_fps  = calculate_fps();
    if (current_fps > 0) {
        snprintf(fps_string, sizeof(fps_string), "FPS: %.2f", current_fps);
    } else {
        snprintf(fps_string, sizeof(fps_string), "FPS: --");
    }
    snprintf(watermark_text, sizeof(watermark_text), "%s | %s", time_string, fps_string);

    // fprintf(stderr, "fps = %d,frame = %d \n", fps,fps_frame_count);
    draw_text_on_yuv(frame->data[0], width, height, watermark_text, width - (strlen(watermark_text) * FONT_WIDTH), 0);
    // 在右上角绘制时间戳
    // draw_text_on_yuv(frame->data[0], width, height, time_string, width - (strlen(time_string) * FONT_WIDTH), 0);
    
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
    
    pthread_t decode_tid, process_tid, display_tid;
    pthread_create(&decode_tid, NULL, decode_thread, fmt_ctx);
    pthread_create(&process_tid, NULL, process_thread, NULL);
    pthread_create(&display_tid, NULL, display_thread, NULL);
#if 0
    while (1) {
        while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            // 退出循环
            return 0;
        }
    }
        SDL_Delay(30);  // 可以稍微延迟以减少CPU占用
    }
#endif

    // 等待线程结束
    pthread_join(decode_tid, NULL);
    pthread_join(process_tid, NULL);
    pthread_join(display_tid, NULL);

exit:
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);
    pthread_mutex_destroy(&process_mutex);
    pthread_cond_destroy(&process_cond);

    sem_post(&sem);
    sem_wait(&sem);
    sem_destroy(&sem);
    close_sdl();
    avformat_close_input(&fmt_ctx);
    fmt_ctx = NULL;
    return 0;
}
