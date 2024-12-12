#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <semaphore.h>
#include "client.h"

void client_write(SharedMemory *shared_memory, const char *data) {
    int value;
    sem_wait(&shared_memory->semaphore_w);

    int data_len = strlen(data) + 1;
    strcpy(shared_memory->buffer + shared_memory->write_index, data);
    shared_memory->write_index = (shared_memory->write_index + strlen(data) + 1);
    if (BUFFER_SIZE - shared_memory->write_index < (strlen(data) + 1)) {
        shared_memory->write_index = 0;
    }
    sem_post(&shared_memory->semaphore_r);
}

char *client_read(SharedMemory *shared_memory) {
    int value;
    sem_wait(&shared_memory->semaphore_r); // 等待信号量
    // print(" shared_memory->read_index = %d ,  shared_memory->write_index = %d", shared_memory->read_index,shared_memory->write_index);
    char *data = shared_memory->buffer + shared_memory->read_index;
    shared_memory->read_index = shared_memory->write_index;
    sem_post(&shared_memory->semaphore_w);
    return data;
}

void init_shared_memory(SharedMemory **shared_memory) {
    int shmid;
    key = ftok("shmfile", 65);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    shmid = shmget(key, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    *shared_memory = shmat(shmid, NULL, 0);
    if (*shared_memory == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    (*shared_memory)->write_index = 0;
    (*shared_memory)->read_index = 0;
}

void destroy_shared_memory(SharedMemory *shared_memory) {
    sem_destroy(&shared_memory->semaphore_r);
    sem_destroy(&shared_memory->semaphore_w);

    shmdt(shared_memory);
    shmctl(shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666), IPC_RMID, NULL);
}

void client() {
    int shmid;
    int msg_error_count = 0;
    char *shm, *s;
    SharedMemory *shared_memory;
    char data[10];
#if 1
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
// 初始化缓冲区索引
    shared_memory->write_index = 0;
    shared_memory->read_index = 0;
#else
    key = ftok("shmfile", 65);
    init_shared_memory(&shared_memory);
#endif
long int i=0;
long int num;
    while (1) {
        int *s = client_read(shared_memory);
       // if(s!=NULL){
            // printf("Server : %s\n", s);
         #if 0
            //num = atoi(s);
            if(i!=s){
                printf("Error: %ld %s \n",i,s);
                while(1);
            }
            i++;
        // }
    #endif
    }

    exit(EXIT_SUCCESS);
}

int main() {
    client();
    return 0;
}
