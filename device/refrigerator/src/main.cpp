#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include "../../include/message.h"
#include "video.h"

int isAdded = -1;//设备是否添加
char macAddr[32] = "02:11:22:33:44:55:66:77";
// #define DEBUG
static int GetCurSeq(void)
{
	static int cur_seq = 0;
	cur_seq++;
	if(cur_seq >= 0xFF)
	{
		cur_seq = 0;
	}
	return cur_seq;
}

static int GetChkSum(char* data, int dataLen)
{
	int sum = 0;
	for(int i = 0; i < dataLen; i++)
	{
		sum += data[i];
	}
	return sum;
}

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
	for(int i = 0; i < msg->total_len-msg->header_len*4; i++)
	{
		printf("Msg data[%d]:0x%x\n", i, msg->data[i]);
	}
#endif
}

int DoAddDevice(int sockfd, MsgStruct *msg, char *addr)
{
	int preSeq = GetCurSeq();
	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_D2S;
	msg->data_type = MSG_DATA_ADD_DEVICE;
	msg->seq_num = preSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->custom1 = 0xFF;
	msg->custom2 = 0xFF;
	msg->total_len = MSG_HEADER_STABLE_LEN*4+strlen(addr);
	msg->source_addr = 0x7F000001;//GetIpMun(sourceIp)
	msg->target_addr = 0x7F000001;//GetIpMun(targetIp)
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	memcpy(msg->data, addr, strlen(addr));

	if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return -1;
	}
	printf("Add device request send success!\n");
	memset(msg, 0x0, sizeof(MsgStruct));
	if(recv(sockfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("Fail to recv.\n");
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
	if(msg->protocol_type != MSG_PROTOCOL_S2D)
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
		printf("Congratulations！You added successfully!\n");
		int flashfile_fd = open("./flashfile", O_RDWR|O_CREAT);
		if (flashfile_fd == -1)
		{
			perror("fail to open\n");
			return -1;
		}
		char add[2] = {'Y'};
		if (write(flashfile_fd, add, 1) == -1)
		{
			perror("fail to read flashfile\n");
			return -1;
		}
		close(flashfile_fd);
	}
	else if(msg->custom1 == MSG_CUSTOM1_ADD_DEVICE_FAILURE)
	{
		printf("Sorry！You added failed:\n");
		switch (msg->custom2)
		{
			case MSG_CUSTOM2_ADD_DEVICE_FAILURE_MACADDR:
				printf("Mac address error.\n");
				break;
			case MSG_CUSTOM2_ADD_DEVICE_FAILURE_ALREADY:
				printf("Device already added.\n");
				break;
			case MSG_CUSTOM2_ADD_DEVICE_FAILURE_CONNECT:
				printf("Connection failed.\n");
				break;
			case MSG_CUSTOM2_ADD_DEVICE_FAILURE_DATABASE:
				printf("Database error.\n");
				break;	
			case MSG_CUSTOM2_ADD_DEVICE_FAILURE_DEVICE:
				printf("Device error.\n");
				break;		
			default:
				printf("unknow reason:0x%x\n", msg->custom2);
				break;
		}
		return -1;
	}
	else
	{
		printf("Return unknow status:0x%x\n",msg->custom1);
		return -1;
	}
	
	return 0;
}

int DoConDevice(int sockfd, MsgStruct *msg, char *addr)
{
	//1.组装数据包
	//2.发送给服务器
	//3.获取服务器返回数据
	printf("To connect server!\n");
	int preSeq = GetCurSeq();
	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_D2S;
	msg->data_type = MSG_DATA_DEVICE_CONNECTION;
	msg->seq_num = preSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->custom1 = 0xFF;
	msg->custom2 = 0xFF;
	msg->total_len = MSG_HEADER_STABLE_LEN*4+strlen(addr);
	msg->source_addr = 0x7F000001;//GetIpMun(sourceIp)
	msg->target_addr = 0x7F000001;//GetIpMun(targetIp)
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	memcpy(msg->data, addr, strlen(addr));

	if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return -1;
	}
	printf("Connect request Send success!\n");
	memset(msg, 0x0, sizeof(MsgStruct));
	if(recv(sockfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("Fail to recv.\n");
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
	if(msg->protocol_type != MSG_PROTOCOL_S2D)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
		return -1;		
	}
	if(msg->data_type != MSG_DATA_DEVICE_CONNECTION)
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
	if(msg->custom1 == MSG_CUSTOM1_DEVICE_CONNECTION_SUCCESS)
	{
		printf("Congratulations！You connection successfully!\n");
	}
	else if(msg->custom1 == MSG_CUSTOM1_DEVICE_CONNECTION_FAILURE)
	{
		printf("Sorry！You connection failed:\n");
		switch (msg->custom2)
		{
			case MSG_CUSTOM2_DEVICE_CONNECTION_FAILURE_MAC:
				printf("Mac address error.\n");
				break;
			case MSG_CUSTOM2_DEVICE_CONNECTION_FAILURE_NOTADD:
				printf("Device not added.\n");
				break;
			case MSG_CUSTOM2_DEVICE_CONNECTION_FAILURE_NET:
				printf("Connection failed.\n");
				break;		
			default:
				printf("unknow reason:0x%x\n", msg->custom2);
				break;
		}
		return -1;
	}
	else
	{
		printf("Return unknow status:0x%x\n",msg->custom1);
		return -1;
	}
	
	return 0;
}

int DoAlarm(int sockfd, MsgStruct *msg, char *addr, int alarmType)
{
	//1.组装数据包
	//2.发送给服务器
	//3.获取服务器返回数据---无返回数据
	int preSeq = GetCurSeq();
	msg->version = MSG_VERSION;											
	msg->header_len = MSG_HEADER_STABLE_LEN;							
	msg->encrypt_type = MSG_ENCRYPT_NONE;								
	msg->protocol_type = MSG_PROTOCOL_D2S;								
	msg->data_type = MSG_DATA_DEVICE_ALARM_NOTIFY;						
	msg->seq_num = preSeq;												
	msg->frag_flag = MSG_FLAG_FRAG_NO;									
	msg->frag_offset = 0;												
	msg->custom1 = MSG_CUSTOM1_DEVICE_ALARM_NOTIFY_REFRIGERATOR;		
	msg->custom2 = alarmType;											
	msg->total_len = MSG_HEADER_STABLE_LEN*4+strlen(addr);				
	msg->source_addr = 0x7F000001;//GetIpMun(sourceIp)					
	msg->target_addr = 0x7F000001;//GetIpMun(targetIp)					
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);	
	memcpy(msg->data, addr, strlen(addr));

	if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("Alarm Msg fail to send.\n");
		return -1;
	}
	return 0;
}

int SetDeviceStatus(int acceptfd, MsgStruct *msg)
{
	//1.检查参数
	//2.从接收到客户端的数据包中获取“设置命令”
	//3.可以模拟数据
	//4.组装数据包，返回给服务器
	printf("Set Device Status!\n");
	PrintMsg(msg);
	int wrong_code = -1;
	int preSeq = msg->seq_num;
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
	if(msg->protocol_type != MSG_PROTOCOL_S2D)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
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
	if(msg->custom1 != MSG_CUSTOM1_SET_DEVICE_REFRIGERATOR )
	{
		printf("Device is not Refrigerator!\n");
		wrong_code = 1;
	}
	char mac[32] = {0};
	strncpy(mac,msg->data,23);
	if(strcmp(mac,macAddr) != 0)
	{
		wrong_code = 0;
	}
	int leftvalue_len = strlen(msg->data) -23;
	char leftvalue[16] = {0};
	if(leftvalue_len != 0)
	{
		strcpy(leftvalue,msg->data+23);
		printf("leftvalue=%s\n", leftvalue);
	}

	if(wrong_code == -1)
	{
		switch (msg->custom2)
		{
			case MSG_CUSTOM2_SET_DEVICE_REF_FRESH :
				printf("Refrigerator Fresh Set:%s.\n", leftvalue);
				break;
			case MSG_CUSTOM2_SET_DEVICE_REF_FREEZ:
				printf("Refrigerator Freez Set:%s.\n", leftvalue);
				break;
			default:
				printf("unknow setting:0x%x\n", msg->custom2);
				wrong_code = 1;
		}

	}
	//组装数据
	preSeq = GetCurSeq();
	memset(msg, 0x0, sizeof(MsgStruct));
	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_D2S;
	msg->data_type = MSG_DATA_SET_DEVICE_STATUS;
	msg->seq_num = preSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;	
	msg->total_len = MSG_HEADER_STABLE_LEN*4;
	msg->source_addr = 0x7F000001;//GetIpMun(sourceIp)
	msg->target_addr = 0x7F000001;//GetIpMun(targetIp)
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	if(wrong_code == -1)
	{
		msg->custom1 = 0;
		msg->custom2 = 0xFF;
	}
	else
	{
		msg->custom1 = 1;
		msg->custom2 = wrong_code;
	}

	if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("Set Device Status Msg fail to send.\n");
		return -1;
	}
	printf("Set Device Status Msg send success.\n");
	return 0;
}

static char* simulateFresh[5] = {"1", "2", "3", "4", "5"};
static char* simulateFreez[5] = {"-11", "-12", "-13", "-14", "-15"};
static int GetSimulateData(int msgType, char* msgData)
{
	switch (msgType)
	{
		case MSG_CUSTOM2_GET_DEVICE_REF_FRESH:
			strcpy(msgData, simulateFresh[random()%5]);
			printf("msgData:%s\n", msgData);
			break;
		case MSG_CUSTOM2_GET_DEVICE_REF_FREEZ:
			strcpy(msgData, simulateFreez[random()%5]);
			printf("msgData:%s\n", msgData);
			break;	
		default:
			printf("unknow setting:0x%x\n", msgType);
			break;
	}
	return 0;
}

int GetDeviceStatus(int acceptfd, MsgStruct *msg)
{
	//1.检查参数
	//2.从接收到客户端的数据包中获取“读取命令”
	//3.调用uart接口从传感器中获取（可以模拟）
	//4.组装数据包，返回给服务器
	PrintMsg(msg);
	int wrong_code = -1;
	int preSeq = msg->seq_num;
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
	if(msg->protocol_type != MSG_PROTOCOL_S2D)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
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
	if(msg->custom1 != MSG_CUSTOM1_GET_DEVICE_REFRIGERATOR)
	{
		printf("Device is not Refrigerator!\n");
		wrong_code = 1;
	}
	char mac[32] = {0};
	strncpy(mac,msg->data,23);
	if(strcmp(mac,macAddr) != 0)
	{
		wrong_code = 0;
	}
	int get_order = msg->custom2;

	memset(msg, 0x0, sizeof(MsgStruct));
	if(wrong_code == -1)
	{
		printf("Refrigerator Device State Get\n");
		switch (get_order)
		{
			case MSG_CUSTOM2_GET_DEVICE_REF_FRESH:
				GetSimulateData(get_order, msg->data);
				printf("Refrigerator Fresh:%s.\n",msg->data);				
				break;
			case MSG_CUSTOM2_GET_DEVICE_REF_FREEZ:
				GetSimulateData(get_order, msg->data);
				printf("Refrigerator Freez:%s\n", msg->data);
				break;		
			default:
				printf("unknow setting:0x%x\n", get_order);
				wrong_code = 1;
		}

	}
	//组装数据
	preSeq = GetCurSeq();
	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_D2S;
	msg->data_type = MSG_DATA_GET_DEVICE_STATUS;
	msg->seq_num = preSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;	
	msg->total_len = MSG_HEADER_STABLE_LEN*4+strlen(msg->data);
	msg->source_addr = 0x7F000001;//GetIpMun(sourceIp)
	msg->target_addr = 0x7F000001;//GetIpMun(targetIp)
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	if(wrong_code == -1)
	{
		msg->custom1 = 0;
		msg->custom2 = 0xFF;
	}
	else
	{
		msg->custom1 = 1;
		msg->custom2 = wrong_code;
	}

	if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return -1;
	}

	return 0;
}

static void* DealwithDevServer(void *arg)
{
	printf("%s:IN\r\n", __FUNCTION__);
	int deviceSockFd = *(int *)arg;
	MsgStruct msg;
	memset(&msg, 0x0, sizeof(MsgStruct));
	while(recv(deviceSockFd, &msg, sizeof(msg), 0) > 0)
	{
		switch (msg.data_type)
		{
			//设置设备状态
			case MSG_DATA_SET_DEVICE_STATUS:
				SetDeviceStatus(deviceSockFd, &msg);
				break;
			//读取设备状态
			case MSG_DATA_GET_DEVICE_STATUS:
				GetDeviceStatus(deviceSockFd, &msg);
				break;			
			default:
				break;
		}
		memset(&msg, 0x0, sizeof(MsgStruct));
	}

	printf("server exit.\n");
	close(deviceSockFd);
	exit(0);
	return NULL;
}

static void* DealwithAlarmServer(void *arg)
{
	printf("%s:IN\r\n", __FUNCTION__);
	int alarmSockFd = *(int *)arg;
	MsgStruct msg;
	int alarmType = -1;
	int i = 0;
	while (1)
	{
		memset(&msg, 0x0, sizeof(MsgStruct));

		//填充数据包，发送告警消息MSG_DATA_DEVICE_ALARM_NOTIFY给告警服务器
		//Tips:不需要从服务器返回消息
		sleep(5);
		if ( i % 3 == 0)
		{
			alarmType = MSG_CUSTOM2_DEVICE_ALARM_NOTIFY_REF_FOOD;
			printf("MSG_CUSTOM2_DEVICE_ALARM_NOTIFY_REF_FOOD.\n");
		}
		else if (i % 3 == 1)
		{
			alarmType = MSG_CUSTOM2_DEVICE_ALARM_NOTIFY_REF_FRESH;
			printf("MSG_CUSTOM2_DEVICE_ALARM_NOTIFY_REF_FRESH.\n");
		}
		else if (i % 3 == 2)
		{
			alarmType = MSG_CUSTOM2_DEVICE_ALARM_NOTIFY_REF_FREEZ;
			printf("MSG_CUSTOM2_DEVICE_ALARM_NOTIFY_REF_FREEZ.\n");
		}
		DoAlarm(alarmSockFd,&msg,macAddr,alarmType);
		i++;
		sleep(60);
	}
	
	printf("thead exit.\n");
	close(alarmSockFd);
	exit(0);
	return NULL;
}

// ./refrigerator
int main(int argc, const char *argv[])
{
	int deviceSockFd = -1;
	int alarmSockFd = -1;
	int mediaSockFd = -1;

    pthread_t threadDev_t;
	pthread_t threadAlarm_t;

	struct sockaddr_in serveraddr;
	MsgStruct msg;
	memset(&msg, 0x0, sizeof(MsgStruct));

	//1.Connect Device Server
	if((deviceSockFd = socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serveraddr.sin_port = htons(SERVER_DEVICE_PORT);

	if(connect(deviceSockFd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("fail to connect");
		return -1;
	}

	//2.Connect Alarm Service
	if((alarmSockFd = socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serveraddr.sin_port = htons(SERVER_ALARM_PORT);

	if(connect(alarmSockFd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("fail to connect\n");
		return -1;
	}
	
	//3.Connect Media Server
	if((mediaSockFd = socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serveraddr.sin_port = htons(SERVER_MEDIA_PORT);

	if(connect(mediaSockFd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("fail to connect");
		return -1;
	}

	//3.Todo:读取flashfile文件获取设备是否已添加，N表示未添加，Y表示已添加
	//Todo:读取flashfile文件判断设备是否已被添加,赋值isAdded
    int flashfile_fd = open("./flashfile", O_RDONLY|O_CREAT);
    if (flashfile_fd == -1)
    {
        perror("fail to open\n");
        return -1;
    }
    char add[2] = {0};
    if (read(flashfile_fd, add, 1) == -1)
    {
        perror("fail to read flashfile\n");
        return -1;
    }
    close(flashfile_fd);

    if (add[0] == 'Y')
    {
        printf("Device added\n");
        isAdded = 0;
    }
    else
    {
        printf("Device not added\n");
    }
	
	while(1)
	{
		if(isAdded == -1)//未被添加
		{
			//发送MSG_DATA_ADD_DEVICE消息给设备管理服务器
			if(DoAddDevice(deviceSockFd, &msg, macAddr) == 0)
			{
				isAdded = 0;
				break;
			}
		}
		else if(isAdded == 0)//已被添加
		{
			//发送MSG_DATA_DEVICE_CONNECTION消息给设备管理服务器
			if(DoConDevice(deviceSockFd, &msg, macAddr) == 0)
			{
				break;
			}
		}
		sleep(3);
	}

	//线程内接收设备管理服务器读取设备状态、设置设备状态
	pthread_create(&threadDev_t, NULL, DealwithDevServer, &deviceSockFd);
	//线程内接收串口消息，并上传告警服务器
	pthread_create(&threadAlarm_t, NULL, DealwithAlarmServer, &alarmSockFd);

	//启动live555
	StartVideo(&mediaSockFd);

	while(1)
	{
		sleep(5);
	}

	return 0;
}
