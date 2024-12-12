#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <semaphore.h>

char* get_cur_time() {
static char s[20];
time_t t;
struct tm* ltime;
struct timespec ts;
time(&t);
ltime = localtime(&t);
clock_gettime(CLOCK_REALTIME, &ts);
strftime(s, 20, "%Y-%m-%d %H:%M:%S", ltime);
snprintf(s + strlen(s), 6, ".%06ld", ts.tv_nsec / 1000);
return s;
}

#define BUFFER_SIZE 1024
#define CLIENT_NAME "server"
#define print(format, ...) printf("%s  [%s] %s:%d: " format "\n",get_cur_time(), CLIENT_NAME, __func__, __LINE__, ##__VA_ARGS__)

typedef struct {
    char buffer[BUFFER_SIZE];
    int write_index;
    int read_index;
    sem_t semaphore_r;
    sem_t semaphore_w;
} SharedMemory;