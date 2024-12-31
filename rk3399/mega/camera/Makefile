# 文件夹名
FOLDER_NAME := out
CURENT_DIR := $(shell pwd)

# 编译器
CC = gcc

# 编译选项
CFLAGS = -Wall -Wno-unused-variable -Wno-implicit-function-declaration

# 头文件路径
INCLUDE_PATH = -I/usr/include/ffmpeg -I/usr/include/SDL2

# 库文件路径
LIBRARY_PATH = -L/usr/lib/ffmpeg -L/usr/lib

# 链接的库
LIBS = -lavformat -lavcodec -lavutil -lswscale -lm -lSDL2

# 目标文件名
TARGET = get_frame

# 源文件
SRCS = get_frame.c

# 目标文件名
OBJS = $(SRCS:.c=.o)

# 默认目标
all: $(TARGET)

# 编译源文件生成可执行文件
$(TARGET): $(SRCS)
	mkdir -p $(FOLDER_NAME)
	$(CC) $(CFLAGS) $(INCLUDE_PATH) $(LIBRARY_PATH) -o $@ $^ $(LIBS)
	mv $@ $(FOLDER_NAME)

# 清理生成的文件
clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf $(FOLDER_NAME)
