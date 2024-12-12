# 文件夹名
FOLDER_NAME := out
CURENT_DIR := $(shell pwd)
# 编译器
CC = gcc
# 编译选项
CFLAGS = -Wall -Wno-unused-variable -Wno-implicit-function-declaration
# 目标文件名
TARGET = main
# 源文件
SRCS = main.c
# 目标文件名
OBJS = $(SRCS:.c=.o)
# 默认目标
all: $(TARGET)
# 编译源文件生成可执行文件
$(TARGET): $(SRCS)
	mkdir -p $(FOLDER_NAME)
	$(CC) $(CFLAGS) -o $@ $^
	mv $@ $(FOLDER_NAME)
	$(MAKE) -C server FOLDER_NAME=$(FOLDER_NAME) CURENT_DIR=$(CURENT_DIR)
	$(MAKE) -C client FOLDER_NAME=$(FOLDER_NAME) CURENT_DIR=$(CURENT_DIR)

# 清理生成的文件
clean:
	rm -f $(OBJS) $(TARGET)
	$(MAKE) -C server clean
	$(MAKE) -C client clean
	rm -rf $(FOLDER_NAME)