#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#define DTC_CAMERA 0xFF030000
pthread_mutex_t pcon_lock; // 定义一个互斥锁
pthread_cond_t cond;

typedef enum
{
    MAXIM_LINK_A,
    MAXIM_LINK_B,
    MAXIM_LINK_C,
    MAXIM_LINK_D,
}maxim_link_id_t;

static int pre_status = 0 ;
static int status_oms_front = 0 ;
static int status_oms_rear = 0;

static int errcode_status = 0;
static int errcode_num = 0;
static int errcode_cont = 0;

#define ERR_CODE_CAMERA_DVR 0xFE030101
#define ERR_CODE_CAMERA_DMS 0xFE030201
#define ERR_CODE_CAMERA_OMS 0xFE030301

void msleep(long msec) {
    struct timespec ts;
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
}

void* pcond_phread(void* arg) {
    while(1) {
        pthread_mutex_lock(&pcon_lock);
        pthread_cond_wait(&cond, &pcon_lock);
        camera_report_errcode(errcode_num,errcode_status);
        pthread_mutex_unlock(&pcon_lock);
    }
    return NULL;
}

// 0 0
// 0 1
// 1 0
// 1 1

void* errcode_report_phread(void* arg) {
    while (1) {
        printf("input status errcode_num errcode_status :");
        scanf(" %d%d",&errcode_num ,&errcode_status);
        pthread_cond_signal(&cond);
        msleep(1);
    }
}

void camera_report_errcode(int sensor_id, int status) {
    int errcode = 0;

    printf("sensor_id: %d, status: %d \n", sensor_id, status);
    if(sensor_id == MAXIM_LINK_A)
    {
        status_oms_front =  status;
        return ;
    }

    if(sensor_id == MAXIM_LINK_B)
    {
        status_oms_rear =  status;
        sensor_id = MAXIM_LINK_A;
    }
    status = status_oms_rear || status_oms_front;
    status = !status;
    
    if ( status == 0 )
        return ;
    else 
        errcode_cont++;

    switch(sensor_id)
    {
        case MAXIM_LINK_A:
            errcode = ERR_CODE_CAMERA_OMS;
            if(errcode_cont >= 5)
            {
                printf("[OMS]errcode: 0x%x, status: %d\n", errcode, status);
                errcode_cont = 0;
            }
            // errcode_report(errcode, status);
            break;
        case MAXIM_LINK_B:
            // errcode = ERR_CODE_CAMERA_OMS;
            // SENSOR_ERROR("errcode: %d, status: %d", errcode, status);
            // errcode_report(errcode, status);
            break;
        case MAXIM_LINK_C:
            errcode = ERR_CODE_CAMERA_DMS;
            printf("[DMS]errcode: 0x%x, status: %d\n", errcode, status);
            // errcode_report(errcode, status);
            break;
        case MAXIM_LINK_D:
            errcode = ERR_CODE_CAMERA_DVR;
            printf("[DVR]errcode: 0x%x, status: %d\n", errcode, status);
            // errcode_report(errcode, status);
            break;
        default:
            printf("errcode: 0x%x, status: %d\n", errcode, status);
    }
}

int main(int argc, char *argv[])
{
    int code;
    int chip_num = 1;
    int ch = 0;
    int type = 4;
    int camera_type = 1;
    pthread_t pcond_lphread;
    pthread_t errcode_phread;

    pthread_mutex_init(&pcon_lock, NULL); // 初始化互斥锁

    int result = pthread_create(&pcond_lphread, NULL, pcond_phread, NULL);
    if (result != 0) {
        printf("Error creating thread: %d\n", result);
        return 1;
    }

    result = pthread_create(&errcode_phread, NULL, errcode_report_phread, NULL);
    if (result != 0) {
        printf("Error creating thread: %d\n", result);
        return 1;
    }

    pthread_join(pcond_lphread, NULL);
    pthread_join(errcode_phread, NULL);

#if 0
    for(int i=0;i<256;i++){
        if(i%2 == 0){
            msleep(100);
            printf("********************************************\n");
            printf("********************************************\n");
            printf("********************************************\n");
            printf("********************************************\n");
            printf("********************************************\n");
            printf("********************************************\n");
            printf("********************************************\n");
            printf("********************************************\n");
            printf("********************************************\n");
            printf("********************************************\n");
            printf("********************************************\n");
            printf("********************************************\n");
        }
        else {
            msleep(100);
            printf("\n");
            printf("\n");
            printf("\n");
            printf("\n");
            printf("\n");
            printf("\n");
            printf("\n");
            printf("\n");
            printf("\n");
        }
    }
#endif

#if 0
    code = DTC_CAMERA |camera_type<<12 | ((chip_num + 1) << 8) | (ch << 4) | type;
    printf("code = 0x%x\n",code);
    code = DTC_CAMERA | ((chip_num + 1) << 8) | (ch << 4) | type;
    printf("code = 0x%x\n",code);
#endif 
    return 0;
}

#if 0
    void max_report_dtc_msg(unsigned char chip_num, unsigned char ch, unsigned char type, int state)
{
rvc_cam_diag max20086-1 report dtc (code, state ,ch ,type): (0xff031201, 0, 0, 1)
rvc_cam_diag max20086-1 report dtc (code, state ,ch ,type): (0xff031204, 0, 0, 4)

    //int rc = 0;
    //struct IpcMessage ipc_msg;
    int code = 0x00;

    code = DTC_CAMERA | ((chip_num + 1) << 8) | (ch << 4) | type;

    if(camera_type == 1)
    {
        code = DTC_CAMERA |camera_type<<12 | ((chip_num + 1) << 8) | (ch << 4) | type;
        LOGE("[test_1] rvc_cam_diag max20086-%u report dtc (code, state ,ch ,type): (0x%x, %d, %d, %d)", chip_num, code, state ,ch, type);
    }
    //LOGD("rvc_cam_diag max20086-%u report dtc (code, state ,ch ,type): (0x%x, %d, %d, %d)", chip_num, code, state ,ch, type);
    if( 0 == chip_num){
        LOGE("rvc_cam_diag max20086-%u report dtc (code, state ,ch ,type): (0x%x, %d, %d, %d)", chip_num, code, state ,ch, type);
    }

    report_dtc_msg(code, state);
}

#endif