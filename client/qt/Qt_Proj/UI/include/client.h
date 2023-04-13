#ifndef CLIENT_H
#define CLIENT_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "message.h"
#include <time.h>
// #define DEBUG
#define NAME_PASSWORD_LEN 128
#define DEVICE_STATUS_LEN 10
extern int userSockFd;
extern int deviceSockFd;
extern int alarmSockFd;
// int identifySockFd = -1;
extern int mediaSockFd;


extern struct sockaddr_in serveraddr;
extern int cmd;

extern MsgStruct msg;
extern char name[NAME_PASSWORD_LEN];
extern char focusMac[32];
extern char para[128];
const char sourceIp[32] = "127.0.0.1";
extern char url[128];

extern char aircondition_status_onOff[DEVICE_STATUS_LEN];
extern char aircondition_status_hotCold[DEVICE_STATUS_LEN];
extern char aircondition_status_temp[DEVICE_STATUS_LEN];
extern char aircondition_status_wind[DEVICE_STATUS_LEN];

extern char refrigerator_status_fresh[DEVICE_STATUS_LEN];
extern char refrigerator_status_freez[DEVICE_STATUS_LEN];

extern int GetCurSeq(void);


extern int GetChkSum(char* data, int dataLen);

static void PrintMsg(MsgStruct *msg)
{
#ifdef DEBUG
    printf("Msg version:0x%x\n", msg->version);
    printf("Msg header_len:0x%x\n", msg->header_len);
    printf("Msg encrypt_type:0x%x\n", msg->encrypt_type);
    printf("Msg protocol_type:0x%x\n", msg->protocol_type);
    printf("Msg total_len:0x%x\n", msg->total_len);
    printf("Msg date_type:0x%x\n", msg->data_type);
    printf("Msg seq_num:0x%x\n", msg->seq_num);
    printf("Msg frag_flag:0x%x\n", msg->frag_flag);
    printf("Msg frag_offset:0x%x\n", msg->frag_offset);
    printf("Msg custom1:0x%x\n", msg->custom1);
    printf("Msg custom2:0x%x\n", msg->custom2);
    printf("Msg header_chk:0x%x\n", msg->header_chk);
    printf("Msg source_addr:0x%x\n", msg->source_addr);
    printf("Msg target_addr:0x%x\n", msg->target_addr);
    for(int i = 0; i < msg->total_len-msg->header_len; i++)
    {
        printf("Msg data[%d]:0x%x\n", i, msg->data[i]);
    }
#endif
}
extern int DoQuery(int sockfd, MsgStruct *msg, char *name, int *DeviceNum, char *mac);
extern int DoAddDevice(int sockfd, MsgStruct *msg, const char *name, char *mac);
extern int DoGetDeviceStatus(int sockfd, MsgStruct *msg, int deviceType, int readcommand,char *mac);
extern int DoSetDeviceStatus(int sockfd, MsgStruct *msg, int deviceType, int writecommand, char *writevalue, const char *mac);
extern int DoDeviceNotice(int sockfd, MsgStruct *msg, const char *name, char *mac);
/* link video server*/
int ConnectVideoServer(const char* sIP, const int nPort, int* nFd);

/* request video url*/
extern int RequestVideoUrl(const int nFd, char* url);

/* close link */
int UnlinkVideoServer(const int nFd);
#endif // CLIENT_H
