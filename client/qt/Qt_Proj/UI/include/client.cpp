#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "client.h"

int userSockFd = -1;
int deviceSockFd = -1;
int alarmSockFd = -1;
// int identifySockFd = -1;
int mediaSockFd = -1;
MsgStruct msg;
struct sockaddr_in serveraddr;
char name[NAME_PASSWORD_LEN] = {0};
char focusMac[32] = {0};
char para[128] = {0};
char url[128] = {0};
char aircondition_status_onOff[DEVICE_STATUS_LEN];
char aircondition_status_hotCold[DEVICE_STATUS_LEN];
char aircondition_status_temp[DEVICE_STATUS_LEN];
char aircondition_status_wind[DEVICE_STATUS_LEN];

char refrigerator_status_fresh[DEVICE_STATUS_LEN];
char refrigerator_status_freez[DEVICE_STATUS_LEN];
int GetCurSeq(void)
{
    static int cur_seq = 0;
    cur_seq++;
    if(cur_seq >= 0xFF)
    {
        cur_seq = 0;
    }
    return cur_seq;
}

int GetChkSum(char* data, int dataLen)
{
    int sum = 0;
    for(int i = 0; i < dataLen; i++)
    {
        sum += data[i];
    }
    return sum;
}

int DoAddDevice(int sockfd, MsgStruct *msg, const char *name, char *mac)
{
    int preSeq = GetCurSeq();
    msg->version = MSG_VERSION;
    msg->header_len = MSG_HEADER_STABLE_LEN;
    msg->encrypt_type = MSG_ENCRYPT_NONE;
    msg->protocol_type = MSG_PROTOCOL_C2S;
    msg->data_type = MSG_DATA_ADD_DEVICE;
    msg->seq_num = preSeq;
    msg->frag_flag = MSG_FLAG_FRAG_NO;
    msg->frag_offset = 0;
    msg->custom1 = strlen(name);
    msg->custom2 = 0;
    msg->total_len = MSG_HEADER_STABLE_LEN+msg->custom1;
    msg->source_addr = 0x7F000001;//GetIpMun(sourceIp)
    msg->target_addr = 0x7F000001;//GetIpMun(targetIp)
    msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
    memcpy(msg->data, name, msg->custom1);

    if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
    {
        perror("fail to send.\n");
        return -1;
    }

    memset(msg, 0x0, sizeof(MsgStruct));
    if(recv(sockfd, msg, sizeof(MsgStruct), 0) < 0)
    {
        perror("fail to recv.\n");
        return -1;
    }

    PrintMsg(msg);
    //Check Some Param
    if(msg->version != MSG_VERSION)
    {
        printf("version error:0x%x.\n", msg->version);
        return -1;
    }
    if(msg->encrypt_type != MSG_ENCRYPT_NONE)
    {
        printf("encrypt_type error:0x%x.\n", msg->encrypt_type);
        return -1;
    }
    if(msg->protocol_type != MSG_PROTOCOL_S2C)
    {
        printf("protocol_type error:0x%x.\n", msg->protocol_type);
        return -1;
    }
    if(msg->data_type != MSG_DATA_ADD_DEVICE)
    {
        printf("data_type error:0x%x.\n", msg->data_type);
        return -1;
    }
    if(msg->frag_flag != MSG_FLAG_FRAG_NO)
    {
        printf("frag_flag error:0x%x.\n", msg->frag_flag);
        return -1;
    }
    if(msg->seq_num != preSeq)
    {
        printf("seq error:0x%x; preSeq:0x%x.\n", msg->seq_num, preSeq);
        return -1;
    }
    if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
    {
        printf("header_chk error:0x%x.\n", msg->header_chk);
        return -1;
    }

    //Get Result
    if(msg->custom1 == MSG_CUSTOM1_ADD_DEVICE_SUCCESS)
    {
        printf("New device added successfully!\n");
        strcpy(mac, msg->data);
        return 0;
    }
    else if(msg->custom1 == MSG_CUSTOM1_ADD_DEVICE_FAILURE)
    {
        printf("Sorry！Failed to add device:\n");
        switch (msg->custom2)
        {
            case MSG_CUSTOM2_ADD_DEVICE_FAILURE_MACADDR:
                printf("Wrong mac address.\n");
                break;
            case MSG_CUSTOM2_ADD_DEVICE_FAILURE_ALREADY:
                printf("Device has been added.\n");
                break;
            case MSG_CUSTOM2_ADD_DEVICE_FAILURE_CONNECT:
                printf("Abnormal connection.\n");
                break;
            case MSG_CUSTOM2_ADD_DEVICE_FAILURE_DATABASE:
                printf("Database exception.\n");
                break;
            case MSG_CUSTOM2_ADD_DEVICE_FAILURE_DEVICE:
                printf("Abnormal device.\n");
                break;
            default:
                printf("unknow reason:0x%x\n", msg->custom2);
                break;
        }
    }
    else
    {
        printf("Return unknow status:0x%x\n",msg->custom1);
    }
    memset(msg, 0x0, sizeof(MsgStruct));

    return 0;
}

int DoQuery(int sockfd, MsgStruct *msg, char *name, int *DeviceNum, char *mac)
{
    int preSeq = GetCurSeq();

    msg->version = MSG_VERSION;
    msg->header_len = MSG_HEADER_STABLE_LEN;
    msg->encrypt_type = MSG_ENCRYPT_NONE;
    msg->protocol_type = MSG_PROTOCOL_C2S;
    msg->data_type = MSG_DATA_QUERY_DEVICE;
    msg->seq_num = preSeq;
    msg->frag_flag = MSG_FLAG_FRAG_NO;
    msg->frag_offset = 0;
    msg->custom1 = strlen(name);
    msg->custom2 = 0;
    msg->total_len = MSG_HEADER_STABLE_LEN*4+msg->custom1;
    msg->source_addr = 0x7F000001;//GetIpMun(sourceIp)
    msg->target_addr = 0x7F000001;//GetIpMun(targetIp)
    msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
    memcpy(msg->data, name, msg->custom1);

    if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
    {
        perror("fail to send.\n");
        return -1;
    }

    memset(msg, 0x0, sizeof(MsgStruct));
    if(recv(sockfd, msg, sizeof(MsgStruct), 0) < 0)
    {
        perror("fail to recv.\n");
        return -1;
    }

    PrintMsg(msg);
    //Check Some Param
    if(msg->version != MSG_VERSION)
    {
        printf("version error:0x%x.\n", msg->version);
        return -1;
    }
    if(msg->encrypt_type != MSG_ENCRYPT_NONE)
    {
        printf("encrypt_type error:0x%x.\n", msg->encrypt_type);
        return -1;
    }
    if(msg->protocol_type != MSG_PROTOCOL_S2C)
    {
        printf("protocol_type error:0x%x.\n", msg->protocol_type);
        return -1;
    }
    if(msg->data_type != MSG_DATA_QUERY_DEVICE)
    {
        printf("data_type error:0x%x.\n", msg->data_type);
        return -1;
    }
    if(msg->frag_flag != MSG_FLAG_FRAG_NO)
    {
        printf("frag_flag error:0x%x.\n", msg->frag_flag);
        return -1;
    }
    if(msg->seq_num != preSeq)
    {
        printf("seq error:0x%x; preSeq:0x%x.\n", msg->seq_num, preSeq);
        return -1;
    }
    if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
    {
        printf("header_chk error:0x%x.\n", msg->header_chk);
        return -1;
    }

    //Get Result
    if(msg->custom1 == MSG_CUSTOM1_QUERY_DEVICE_SUCCESS)
    {
        //output all devices' name
        *DeviceNum = msg->custom2;
        strcpy(mac, msg->data);
        return 0;
    }
    else if(msg->custom1 == MSG_CUSTOM1_QUERY_DEVICE_FAILURE)
    {
        printf("Sorry！Failed to query device-names:\n");
        switch (msg->custom2)
        {
            case MSG_CUSTOM2_QUERY_DEVICE_FAILURE_ACCOUNT:
                printf("Wrong account.\n");
                break;
            case MSG_CUSTOM2_QUERY_DEVICE_FAILURE_CONNECT:
                printf("Abnormal connection.\n");
                break;
            case MSG_CUSTOM2_QUERY_DEVICE_FAILURE_DATABASE:
                printf("Database exception.\n");
                break;
            default:
                printf("unknow reason:0x%x\n", msg->custom2);
                break;
        }
    }
    else
    {
        printf("Return unknow status:0x%x\n",msg->custom1);
    }
    memset(msg, 0x0, sizeof(MsgStruct));

    return -1;
}

int DoGetDeviceStatus(int sockfd, MsgStruct *msg, int deviceType, int readcommand,char *mac)
{
    int preSeq = GetCurSeq();
    msg->version = MSG_VERSION;
    msg->header_len = MSG_HEADER_STABLE_LEN;
    msg->encrypt_type = MSG_ENCRYPT_NONE;
    msg->protocol_type = MSG_PROTOCOL_C2S;
    msg->data_type = MSG_DATA_GET_DEVICE_STATUS;
    msg->seq_num = preSeq;
    msg->frag_flag = MSG_FLAG_FRAG_NO;
    msg->frag_offset = 0;
    msg->custom1 = deviceType;
    msg->custom2 = readcommand;
    msg->total_len = MSG_HEADER_STABLE_LEN*4 + strlen(mac);
    msg->source_addr = 0x7F000001;  //GetIpMun(sourceIp)
    msg->target_addr = 0x7F000001;  //GetIpMun(targetIp)
    msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);

    memcpy(msg->data, mac, strlen(mac));

    if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
    {
        perror("fail to send.\n");
        return -1;
    }

    memset(msg, 0x0, sizeof(MsgStruct));
    if (recv(sockfd, msg, sizeof(MsgStruct), 0) < 0)
    {
        perror("fail to recv.\n");
        return -1;
    }

    //Check Some Param
    if (msg->version != MSG_VERSION)
    {
        printf("version error:0x%x.\n", msg->version);
        return -1;
    }
    if (msg->encrypt_type != MSG_ENCRYPT_NONE) {
        printf("encrypt_type error:0x%x.\n", msg->encrypt_type);
        return -1;
    }
    if (msg->protocol_type != MSG_PROTOCOL_S2C) {
        printf("protocol_type error:0x%x.\n", msg->protocol_type);
        return -1;
    }
    if (msg->frag_flag != MSG_FLAG_FRAG_NO) {
        printf("frag_flag error:0x%x.\n", msg->frag_flag);
        return -1;
    }
    if (msg->header_chk != GetChkSum((char*)(msg), msg->header_len)) {
        printf("header_chk error:0x%x.\n", msg->header_chk);
        return -1;
    }

    // 请求失败，打印错误信息
    if (msg->custom1 == MSG_CUSTOM1_GET_DEVICE_STATUS_FAILURE) {
        switch (msg->custom2)
        {
            case MSG_CUSTOM2_GET_DEVICE_STATUS_FAILURE_MACADDR: {printf("macaddr error.\n"); return -1;}
            case MSG_CUSTOM2_GET_DEVICE_STATUS_FAILURE_COMMAND: {printf("command error.\n"); return -1;}
            case MSG_CUSTOM2_GET_DEVICE_STATUS_FAILURE_CONNECT: {printf("connect error.\n"); return -1;}
            case MSG_CUSTOM2_GET_DEVICE_STATUS_FAILURE_DEVICE: {printf("device error.\n"); return -1;}
            default: break;
        }
    }
    if (msg->custom1 == MSG_CUSTOM1_GET_DEVICE_STATUS_SUCCESS)
    {
        switch (deviceType)
        {
            case MSG_CUSTOM1_GET_DEVICE_AIRCONDITION:
            {
                switch (readcommand)
                {
                    // 开关状态和制冷制热状态结果参考设置命令
                    case MSG_CUSTOM2_GET_DEVICE_AIR_ONOFF:
                        memset(aircondition_status_onOff, 0x00, DEVICE_STATUS_LEN);
                        strcpy(aircondition_status_onOff, msg->data);
                        break;
                    case MSG_CUSTOM2_GET_DEVICE_AIR_HOTCOLD:
                        memset(aircondition_status_hotCold, 0x00, DEVICE_STATUS_LEN);
                        strcpy(aircondition_status_hotCold, msg->data);
                        break;
                    case MSG_CUSTOM2_GET_DEVICE_AIR_TEMP:
                        memset(aircondition_status_temp, 0x00, DEVICE_STATUS_LEN);
                        strcpy(aircondition_status_temp, msg->data);
                        break;
                    case MSG_CUSTOM2_GET_DEVICE_AIR_WIND:
                        memset(aircondition_status_wind, 0x00, DEVICE_STATUS_LEN);
                        strcpy(aircondition_status_wind, msg->data);
                        break;
                    default:
                        break;
                }
            } break;
            case MSG_CUSTOM1_GET_DEVICE_REFRIGERATOR:
            {
                switch (readcommand)
                {
                    case MSG_CUSTOM2_GET_DEVICE_REF_FRESH:
                        memset(refrigerator_status_fresh, 0x00, DEVICE_STATUS_LEN);
                        strcpy(refrigerator_status_fresh, msg->data);
                        break;
                    case MSG_CUSTOM2_GET_DEVICE_REF_FREEZ:
                        memset(refrigerator_status_freez, 0x00, DEVICE_STATUS_LEN);
                        strcpy(refrigerator_status_freez, msg->data);
                        break;
                    default: break;
                }
            } break;
            default: break;
        }
        return 0;
    }

    return 0;
}

int DoSetDeviceStatus(int sockfd, MsgStruct *msg, int deviceType, int writecommand, char *writevalue, const char *mac)
{
    int preSeq = GetCurSeq();
    msg->version = MSG_VERSION;
    msg->header_len = MSG_HEADER_STABLE_LEN;
    msg->encrypt_type = MSG_ENCRYPT_NONE;
    msg->protocol_type = MSG_PROTOCOL_C2S;
    msg->data_type = MSG_DATA_SET_DEVICE_STATUS;
    msg->seq_num = preSeq;
    msg->frag_flag = MSG_FLAG_FRAG_NO;
    msg->frag_offset = 0;
    msg->custom1 = deviceType;
    msg->custom2 = writecommand;
    msg->total_len = MSG_HEADER_STABLE_LEN*4+strlen(mac);
    if(writevalue != NULL)
    {
        msg->total_len += strlen(writevalue);
    }
    msg->source_addr = 0x7F000001;
    msg->target_addr = 0x7F000001;
    msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);

    memcpy(msg->data, mac, strlen(mac));
    if(writevalue != NULL)
    {
        strcpy(msg->data+strlen(mac), writevalue);
    }

    if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
    {
        perror("fail to send.\n");
        return -1;
    }

    memset(msg, 0x0, sizeof(MsgStruct));

    if (recv(sockfd, msg, sizeof(MsgStruct), 0) < 0)
    {
        perror("fail to recv.\n");
        return -1;
    }

    //Check Some Param
    if (msg->version != MSG_VERSION) {
        printf("version error:0x%x.\n", msg->version);
        return -1;
    }
    if (msg->encrypt_type != MSG_ENCRYPT_NONE) {
        printf("encrypt_type error:0x%x.\n", msg->encrypt_type);
        return -1;
    }
    if (msg->protocol_type != MSG_PROTOCOL_S2C) {
        printf("protocol_type error:0x%x.\n", msg->protocol_type);
        return -1;
    }
    if (msg->frag_flag != MSG_FLAG_FRAG_NO) {
        printf("frag_flag error:0x%x.\n", msg->frag_flag);
        return -1;
    }
    if (msg->header_chk != GetChkSum((char*)(msg), msg->header_len)) {
        printf("header_chk error:0x%x.\n", msg->header_chk);
        return -1;
    }
    //请求失败，打印错误信息
    if (msg->custom1 == MSG_CUSTOM1_SET_DEVICE_STATUS_SUCCESS) {
        printf("set device success\n");
        return 0;
    }
    if (msg->custom1 == MSG_CUSTOM1_SET_DEVICE_STATUS_FAILURE) {
        switch (msg->custom2)
        {
            case MSG_CUSTOM2_SET_DEVICE_STATUS_FAILURE_MACADDR: {printf("macaddr error.\n"); return -1;}
            case MSG_CUSTOM2_SET_DEVICE_STATUS_FAILURE_COMMAND: {printf("command error.\n"); return -1;}
            case MSG_CUSTOM2_SET_DEVICE_STATUS_FAILURE_CONNECT: {printf("connect error.\n"); return -1;}
            case MSG_CUSTOM2_SET_DEVICE_STATUS_FAILURE_DEVICE: {printf("device error.\n"); return -1;}
            default: break;
        }
    }

    return 0;
}

int DoDeviceNotice(int sockfd, MsgStruct *msg, const char *name, char *mac)
{
    int preSeq = GetCurSeq();
    msg->version = MSG_VERSION;
    msg->header_len = MSG_HEADER_STABLE_LEN;
    msg->encrypt_type = MSG_ENCRYPT_NONE;
    msg->protocol_type = MSG_PROTOCOL_C2S;
    msg->data_type = MSG_DATA_GET_ALARM_NOTICE;
    msg->seq_num = preSeq;
    msg->frag_flag = MSG_FLAG_FRAG_NO;
    msg->frag_offset = 0;
    msg->custom1 = strlen(name);
    msg->custom2 = 0xFF;
    msg->total_len = MSG_HEADER_STABLE_LEN*4+msg->custom1+strlen(mac);
    msg->source_addr = 0x7F000001;
    msg->target_addr = 0x7F000001;
    msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
    strcpy(msg->data, name);
    strcpy(msg->data + msg->custom1, mac);

    if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
    {
        perror("fail to send.\n");
        return -1;
    }
    // printf("send ok!\n");

    memset(msg, 0x0, sizeof(MsgStruct));
    if(recv(sockfd, msg, sizeof(MsgStruct), 0) < 0)
    {
        perror("fail to recv.\n");
        return -1;
    }

    //Check Some Param
    if (msg->version != MSG_VERSION)
    {
        printf("version error:0x%x.\n", msg->version);
        return -1;
    }
    if (msg->encrypt_type != MSG_ENCRYPT_NONE)
    {
        printf("encrypt_type error:0x%x.\n", msg->encrypt_type);
        return -1;
    }
    if (msg->protocol_type != MSG_PROTOCOL_S2C)
    {
        printf("protocol_type error:0x%x.\n", msg->protocol_type);
        return -1;
    }
    if (msg->frag_flag != MSG_FLAG_FRAG_NO)
    {
        printf("frag_flag error:0x%x.\n", msg->frag_flag);
        return -1;
    }
    if (msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
    {
        printf("header_chk error:0x%x.\n", msg->header_chk);
        return -1;
    }
    //请求失败，打印错误信息
    if (msg->custom1 == MSG_CUSTOM1_GET_ALARM_NOTICE_FAILURE)
    {
        switch (msg->custom2)
        {
            case MSG_CUSTOM2_GET_ALARM_NOTICE_ACCOUNT:
            {
                printf("account error.\n");
                return -1;
            }
            case MSG_CUSTOM2_GET_ALARM_NOTICE_CONNECT:
            {
                printf("connect error.\n");
                return -1;
            }
            case MSG_CUSTOM2_GET_ALARM_NOTICE_DATABASE:
            {
                printf("database error.\n");
                return -1;
            }
            default: break;
        }
    }
    if (msg->custom1 == MSG_CUSTOM1_GET_ALARM_NOTICE_SUCCESS)
    {
        printf("Get device notice success\n");
        int noticeNum = msg->custom2;
        printf("NoticeNum=%d\n",noticeNum);
        for(int i=0; i<noticeNum; i++)
        {
            char buffer[50] = {0};
            int noticeLen = 43;
            memcpy(buffer, msg->data+noticeLen*i, noticeLen);
            printf("NoticeData[%d]=%s\n", i, buffer);
        }
        return 0;
    }
    return 0;
}
int _RequestUrl(int fd, char* sUrl)
{
    MsgStruct msg;
    msg.version = MSG_VERSION;
    msg.header_len = MSG_HEADER_STABLE_LEN;
    msg.encrypt_type = MSG_ENCRYPT_NONE;
    msg.protocol_type = MSG_PROTOCOL_C2S;
    msg.data_type = MSG_DATA_GET_PROXY_URL;
    msg.seq_num = 0;
    msg.frag_flag = MSG_FLAG_FRAG_NO;
    msg.frag_offset = 0;
    msg.custom1 = MSG_CUSTOM2_GET_PROXY_URL_DATABASE;
    msg.custom2 = 0xFF;
    msg.source_addr = 0x7F000001;
    msg.target_addr = 0x7F000001;
    msg.total_len = MSG_HEADER_STABLE_LEN*4;
    msg.header_chk = GetChkSum((char*)(&msg), MSG_HEADER_STABLE_LEN);

    int nRet = send(fd, &msg, sizeof(MsgStruct), 0);
    if(nRet <= 0)
    {
        printf("send to server error \n");
    }
    else
    {
        printf("%s %d \n", __func__, __LINE__);
        nRet = recv(fd, &msg, sizeof(MsgStruct), 0);
        if(nRet <= 0)
        {
            printf("recv from server error \n");
        }
        else
        {
            // PrintMsg(&msg);
            memcpy(sUrl, msg.data, msg.total_len - MSG_HEADER_STABLE_LEN*4);
        }
    }
    return nRet > 0 ? 0 : -1;
}

/* link video server*/
int ConnectVideoServer(const char* sIP, const int nPort, int* nFd)
{
    int sock;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
    {
            printf("sock() error \n");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(sIP);
    serv_addr.sin_port = htons(nPort);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
            printf("connect() error!\n");
        return -2;
    }

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    printf ( "%s  connect ip : %s port : %d success\n", asctime (timeinfo), sIP, nPort);

    *nFd = sock;

    return 0;
}

/* request video url*/
int RequestVideoUrl(const int nFd, char* url)
{
    return 	_RequestUrl(nFd, url);
}


/* close link */
int UnlinkVideoServer(const int nFd)
{
    close(nFd);
    return 0;
}
