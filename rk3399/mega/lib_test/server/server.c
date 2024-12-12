#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <semaphore.h>
#include "server.h"

char server_write(SharedMemory *shared_memory, const char *data) {
    int value;
    sem_wait(&shared_memory->semaphore_w);

    int data_len = strlen(data) + 1;
    strcpy(shared_memory->buffer + shared_memory->write_index, data);
    shared_memory->write_index = (shared_memory->write_index + strlen(data) + 1);
    if (BUFFER_SIZE - shared_memory->write_index < (strlen(data) + 1)) {
        shared_memory->write_index = 0;
    }
    sem_post(&shared_memory->semaphore_r);
    return 0;
}

char *server_read(SharedMemory *shared_memory) {
    int value;
    sem_wait(&shared_memory->semaphore_r); // 等待信号量
    // print(" shared_memory->read_index = %d ,  shared_memory->write_index = %d", shared_memory->read_index,shared_memory->write_index);
    char *data = shared_memory->buffer + shared_memory->read_index;
    shared_memory->read_index = shared_memory->write_index;
    sem_post(&shared_memory->semaphore_w);
    return data;
}

void destroy_shared_memory(SharedMemory *shared_memory) {
    sem_destroy(&shared_memory->semaphore_r);
    sem_destroy(&shared_memory->semaphore_w);
    shmdt(shared_memory);
    shmctl(shmget(IPC_PRIVATE, sizeof(SharedMemory), IPC_CREAT | 0666), IPC_RMID, NULL);
}

void server() {
    key_t key;
    int shmid;
    char *shm, *s;
    SharedMemory *shared_memory;
    char data[10];

    key = ftok("shmfile", 65);

    shmid = shmget(key, BUFFER_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    shm = shmat(shmid, (void*)0, 0);
    if (shm == (char *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    shared_memory = (SharedMemory *)shm;
    if (sem_init(&shared_memory->semaphore_r, 1, 0) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&shared_memory->semaphore_w, 1, 1) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
    shared_memory->write_index = 0;
    shared_memory->read_index = 0;
    // memset(shared_memory->buffer,0,sizeof(shared_memory->buffer));

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
     long total_bytes_sent = 1048576*10;
    //long total_bytes_sent = 1000;
#if 1
    s = shm;
    int ret;
    for (long int i = 0; i <= total_bytes_sent; i++) {
        sprintf(data, "%ld", i);
        server_write(shared_memory, data);
        // int *s = server_read(shared_memory);
        // printf("client : %s\n", s);
        // if(strcmp(data, s)) {
        //     printf("Error: %ld %s \n",i,s);
        //     while(1);
        // }
    }
#endif

    // 计算结束时间
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    double rate = total_bytes_sent / elapsed_time; // 字节/秒
    rate = rate / (1024 * 8 ); // 转换为Mbit/s

    printf("Total bytes sent: %ld\n", total_bytes_sent);
    printf("Elapsed time: %.2f seconds\n", elapsed_time);
    printf("Communication rate: %.2f Mbit/second\n", rate);

    destroy_shared_memory(&shared_memory);

    exit(EXIT_SUCCESS);
}
