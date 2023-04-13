#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "../include/message.h"
#include "media.h"

#define NAME_PASSWORD_LEN 128
// #define DEBUG
#define DEVICE_STATUS_LEN 10
char aircondition_status_onOff[DEVICE_STATUS_LEN];
char aircondition_status_hotCold[DEVICE_STATUS_LEN];
char aircondition_status_hotCold[DEVICE_STATUS_LEN];
char aircondition_status_temp[DEVICE_STATUS_LEN];
char aircondition_status_wind[DEVICE_STATUS_LEN];

char refrigerator_status_fresh[DEVICE_STATUS_LEN];
char refrigerator_status_freez[DEVICE_STATUS_LEN];

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

void DoRegister(int sockfd, MsgStruct *msg)
{
	int preSeq = GetCurSeq();
	char name[NAME_PASSWORD_LEN] = {0};
	char password[NAME_PASSWORD_LEN] = {0};
	printf("Input name:\n(Tips: alphabet, number, underscore)\n");
	scanf("%s", name);
	getchar();

	printf("Input passwd:\n(Tips: alphabet, number, underscore)\n");
	scanf("%s", password);
	getchar();

	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_C2S;
	msg->data_type = MSG_DATA_ACCOUNT_REGISTER;
	msg->seq_num = preSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->custom1 = strlen(name);
	msg->custom2 = strlen(password);
	msg->total_len = MSG_HEADER_STABLE_LEN*4+msg->custom1+msg->custom2;
	msg->source_addr = 0x7F000001;//GetIpMun(sourceIp)
	msg->target_addr = 0x7F000001;//GetIpMun(targetIp)
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	memcpy(msg->data, name, msg->custom1);
	memcpy(msg->data + 1 + msg->custom1, password, msg->custom2);

	if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return;
	}

	memset(msg, 0x0, sizeof(MsgStruct));
	if(recv(sockfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("Fail to recv.\n");
		return;
	}
	
	PrintMsg(msg);
	//Check Some Param
	if(msg->version != MSG_VERSION)
	{
		printf("version error:0x%x.\n", msg->version);
		return;		
	}
	if(msg->encrypt_type != MSG_ENCRYPT_NONE)
	{
		printf("encrypt_type error:0x%x.\n", msg->encrypt_type);
		return;		
	}
	if(msg->protocol_type != MSG_PROTOCOL_S2C)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
		return;		
	}
	if(msg->data_type != MSG_DATA_ACCOUNT_REGISTER)
	{
		printf("data_type error:0x%x.\n", msg->data_type);
		return;		
	}
	if(msg->frag_flag != MSG_FLAG_FRAG_NO)
	{
		printf("frag_flag error:0x%x.\n", msg->frag_flag);
		return;		
	}
	if(msg->seq_num != preSeq)
	{
		printf("seq error:0x%x; preSeq:0x%x.\n", msg->seq_num, preSeq);
		return;				
	}
	if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
	{
		printf("header_chk error:0x%x.\n", msg->header_chk);
		return;		
	}

	//Get Result
	if(msg->custom1 == MSG_CUSTOM1_ACCOUNT_REG_SUCCESS)
	{
		printf("Congratulations！You registered successfully!\n");
	}
	else if(msg->custom1 == MSG_CUSTOM1_ACCOUNT_REG_FAILURE)
	{
		printf("Sorry！You registered failed:\n");
		switch (msg->custom2)
		{
			case MSG_CUSTOM2_ACCOUNT_REG_FAILURE_ALREADY:
				printf("Account already exist.\n");
				break;
			case MSG_CUSTOM2_ACCOUNT_REG_FAILURE_CONNECT:
				printf("Connection failed.\n");
				break;
			case MSG_CUSTOM2_ACCOUNT_REG_FAILURE_DATABASE:
				printf("Database error.\n");
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
	
	return;
}

int DoLogin(int sockfd, MsgStruct *msg, char *userName)
{
	int preSeq = GetCurSeq();
	char name[NAME_PASSWORD_LEN] = {0};
	char password[NAME_PASSWORD_LEN] = {0};
	printf("Input name:\n(Tips: alphabet, number, underscore)\n");
	scanf("%s", name);
	getchar();

	printf("Input passwd:\n(Tips: alphabet, number, underscore)\n");
	scanf("%s", password);
	getchar();

	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_C2S;
	msg->data_type = MSG_DATA_ACCOUNT_LOGIN;
	msg->seq_num = preSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->custom1 = strlen(name);
	msg->custom2 = strlen(password);
	msg->total_len = MSG_HEADER_STABLE_LEN*4+msg->custom1+msg->custom2;
	msg->source_addr = 0x7F000001;
	msg->target_addr = 0x7F000001;
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	memcpy(msg->data, name, msg->custom1);
	memcpy(msg->data + 1 + msg->custom1, password, msg->custom2);

	if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return -1;
	}

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
	if(msg->protocol_type != MSG_PROTOCOL_S2C)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
		return -1;		
	}
	if(msg->data_type != MSG_DATA_ACCOUNT_LOGIN)
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
	if(msg->custom1 == MSG_CUSTOM1_ACCOUNT_LOGIN_SUCCESS)
	{
		printf("Login successfully!\n");
		strcpy(userName, name);
		return 0;
	}
	else if(msg->custom1 == MSG_CUSTOM1_ACCOUNT_LOGIN_FAILURE)
	{
		printf("Sorry！Login failed:\n");
		switch (msg->custom2)
		{
			case MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_ACCOUNT:
				printf("Wrong account or account not registered.\n");
				break;
			case MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_PASSWORD:
				printf("Wrong password.\n");
				break;
			case MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_CONNECT:
				printf("Abnormal connection.\n");
				break;	
			case MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_DATABASE:
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
	
	//登陆成果返回0，登陆失败返回-1
	return -1;
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

int DoNameDevice(int sockfd, MsgStruct *msg, const char *name, char *mac, const char *devicename)
{
	int preSeq = GetCurSeq();
    msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_C2S;
	msg->data_type = MSG_DATA_NAME_DEVICE;
	msg->seq_num = preSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->total_len = MSG_HEADER_STABLE_LEN*4;
	msg->source_addr = 0x7F000001;
	msg->target_addr = 0x7F000001;
	msg->header_chk = GetChkSum((char*)(&msg), MSG_HEADER_STABLE_LEN);
    msg->custom1 = strlen(name);
	msg->custom2 = strlen(devicename);

    strcpy(msg->data, name);
	strcpy(msg->data + msg->custom1, mac);
	strcpy(msg->data + msg->custom1 + msg->custom2, devicename);

    if(send(sockfd, &msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return -1;
	}
	printf("send ok!\n");
    memset(&msg, 0x0, sizeof(MsgStruct));

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
	if (msg->header_chk != GetChkSum((char*)(&msg), msg->header_len)) 
	{
		printf("header_chk error:0x%x.\n", msg->header_chk);
		return -1;
	}
	//请求失败，打印错误信息
	if (msg->custom1 == MSG_CUSTOM1_NAME_DEVICE_FAILURE) 
	{
		switch (msg->custom2) 
		{
			case MSG_CUSTOM2_NAME_DEVICE_FAILURE_FORMAT: 
			{
				printf("devicename format error.\n");
				return -1;
			}
			case MSG_CUSTOM2_NAME_DEVICE_FAILURE_CONNECT: 
			{
				printf("connect error.\n");
				return -1;
			}
			case MSG_CUSTOM2_NAME_DEVICE_FAILURE_DATABASE: 
			{
				printf("database error.\n");
				return -1;
			}
			default: break;
		}
	}
	if (msg->custom1 == MSG_CUSTOM1_NAME_DEVICE_SUCCESS) 
	{
		printf("name device success\n");
		return 0;
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
		// printf("Get device notice success\n");
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
	msg->total_len = MSG_HEADER_STABLE_LEN*4+strlen(mac);
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

// ./linux_client
int main(int argc, const char *argv[])
{
	int userSockFd = -1;
	int deviceSockFd = -1;
	int alarmSockFd = -1;
	// int identifySockFd = -1;
	int mediaSockFd = -1;

	struct sockaddr_in serveraddr;
	int cmd;
	MsgStruct msg;
	memset(&msg, 0x0, sizeof(MsgStruct));

	char name[NAME_PASSWORD_LEN] = {0};
	char focusMac[32] = {0};
	char para[16] = {0};
	const char sourceIp[32] = "127.0.0.1";
	char url[128] = {0};

	//1.Connect User Server
	if((userSockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(sourceIp);
	serveraddr.sin_port = htons(SERVER_USER_PORT);

	if(connect(userSockFd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("fail to connect");
		return -1;
	}

	//Todo
	//2.Connect Device Service
	if((deviceSockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(sourceIp);
	serveraddr.sin_port = htons(SERVER_DEVICE_PORT);

	if(connect(deviceSockFd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("fail to connect");
		return -1;
	}

	//3.Connect Alarm Service
	if((alarmSockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(sourceIp);
	serveraddr.sin_port = htons(SERVER_ALARM_PORT);

	if(connect(alarmSockFd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("fail to connect");
		return -1;
	}

	//4.Connect Identify Service
	//5.Connect Media Service
	if((mediaSockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(sourceIp);
	serveraddr.sin_port = htons(SERVER_MEDIA_PORT);

	if(connect(mediaSockFd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("fail to connect");
		return -1;
	}

	while(1)
	{
		printf("*****************************************************************\n");
		printf("*        0.quit          1.register          2.login            *\n");
		printf("*****************************************************************\n");
		printf("Please choose:");

		scanf("%d", &cmd);
		getchar();
		switch(cmd)
		{
            case 0:
                close(userSockFd);
				close(deviceSockFd);
				close(alarmSockFd);
				close(mediaSockFd);
                exit(0);
                break;
            case 1:
				memset(&msg, 0x0, sizeof(MsgStruct));
                DoRegister(userSockFd, &msg);
                break;
            case 2:
				memset(&msg, 0x0, sizeof(MsgStruct));
                if(DoLogin(userSockFd, &msg, name) == 0)
                {
                    goto FIRST_MENU;
                }
                break;
            default:
                printf("Invalid data cmd.\n");
				break;
		}

	}

FIRST_MENU:
	while(1)
	{
		printf("*****************************************************\n");
		printf("0.quit   1.logout	2.add device\n");
		//查询当前用户下所有设备名字并显示
		int DeviceNum = 0;
		char getMac[1024] = {0};
		memset(&msg, 0x0, sizeof(MsgStruct));
		DoQuery(deviceSockFd, &msg, name, &DeviceNum, getMac);//该函数中依次输出设备mac地址，并获取全部设备个数
		for (int i = 0; i < DeviceNum; i++) {
			char deviceMac[25] = {0};
			for (int j = 0; j < 23; j++) {
				deviceMac[j] = getMac[i * 23 + j];
			}
			char deviceType[10] = {0};
			if (deviceMac[0] == '0' && deviceMac[1] == '1')
			{
				strcpy(deviceType, "Aircondition");
			}
			else if (deviceMac[0] == '0' && deviceMac[1] == '2')
			{
				strcpy(deviceType, "Refrigerator");
			}
			printf("%d.%s(%s)\n", i+3, deviceMac, deviceType);// printf("  2.%s   3.%s    *\n");
		}
		printf("*****************************************************\n");
		printf("Please choose:");
		scanf("%d", &cmd);
		// getchar();

		if (cmd  == 0) {
			close(userSockFd);
			close(deviceSockFd);
			close(alarmSockFd);
			close(mediaSockFd);
            exit(0);
		}
		if (cmd == 1) {
			break;
		}
		if (cmd == 2) {
			memset(focusMac, 0x0, 32);
			memset(&msg, 0x0, sizeof(MsgStruct));
			printf("Please run the device. I'm waiting...\n");
			DoAddDevice(deviceSockFd, &msg, name, focusMac);
			printf("Add mac:	%s.\n", focusMac);
			if (focusMac[0] == '0' && focusMac[1] == '1')
			{
				goto SECOND_MENU_AIRCONDITION;
			}
			else if (focusMac[0] == '0' && focusMac[1] == '2')
			{
				goto SECOND_MENU_REFRIGERATOR;
			}
			else 
			{
				printf("Failed to add this device:%s\n", focusMac);
				close(userSockFd);
				close(deviceSockFd);
				close(alarmSockFd);
				close(mediaSockFd);
            	exit(0);
			}
		} else if (cmd > 2 && cmd <= 3 + DeviceNum) {
			memset(focusMac, 0x0, 32);
			for (int i = 0; i < 23; i++) {
				focusMac[i] = getMac[(cmd - 3) * 23 + i];
			}

			if (focusMac[0] == '0' && focusMac[1] == '1')
			{
				goto SECOND_MENU_AIRCONDITION;
			}
			else if (focusMac[0] == '0' && focusMac[1] == '2')
			{
				goto SECOND_MENU_REFRIGERATOR;
			}
			else
			{
				printf("Failed to query this device:%s\n", focusMac);
				close(userSockFd);
				close(deviceSockFd);
				close(alarmSockFd);
				close(mediaSockFd);
            	exit(0);
			}
		} else {
			printf("Invalid data cmd.\n");
		}

SECOND_MENU_AIRCONDITION:
		sleep(1);
		memset(&msg, 0x0, sizeof(MsgStruct));
		DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_AIRCONDITION, MSG_CUSTOM2_GET_DEVICE_AIR_ONOFF, focusMac);
		memset(&msg, 0x0, sizeof(MsgStruct));
		DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_AIRCONDITION, MSG_CUSTOM2_GET_DEVICE_AIR_HOTCOLD, focusMac);
		memset(&msg, 0x0, sizeof(MsgStruct));
		DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_AIRCONDITION, MSG_CUSTOM2_GET_DEVICE_AIR_TEMP, focusMac);
		memset(&msg, 0x0, sizeof(MsgStruct));
		DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_AIRCONDITION, MSG_CUSTOM2_GET_DEVICE_AIR_WIND, focusMac);
		while (1) {
			printf("**************************AIRCONDITION***************************\n");
			printf("*   0.quit	1.return	2.on/off:%s	3.mode:%s	4.temperature:%s	5.Wind:%s	6.Notice\n",\
			 		aircondition_status_onOff, 
			 		aircondition_status_hotCold, 
			 		aircondition_status_temp, 
			 		aircondition_status_wind);
			//分析data中数据参数
			
			printf("*****************************************************\n");
			memset(para, 0x00, 16);
			memset(&msg, 0x0, sizeof(MsgStruct));
			printf("Please choose:");
			scanf("%d", &cmd);
			getchar();
			switch (cmd) {
				case 0:
					close(userSockFd);
					close(deviceSockFd);
					close(alarmSockFd);
					close(mediaSockFd);
            		exit(0);
				case 1:
					goto FIRST_MENU;
					break;
				case 2:
					printf("Please input: on/off\n");
					scanf("%s", para);
					getchar();
					if(strcmp(para, "on") == 0)
					{
						DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_ON, NULL, focusMac);
						memset(aircondition_status_onOff, 0x0, DEVICE_STATUS_LEN);
						strcpy(aircondition_status_onOff, "on");
					}
					else if(strcmp(para, "off") == 0)
					{
						DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_OFF, NULL, focusMac);
						memset(aircondition_status_onOff, 0x0, DEVICE_STATUS_LEN);
						strcpy(aircondition_status_onOff, "off");
					}
					else{
						printf("error input:%s\n",para);
					}
					break;
				case 3:
					printf("Please input: hot/cold\n");
					scanf("%s", para);
					getchar();
					if(strcmp(para, "hot") == 0)
					{
						DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_HOT, NULL, focusMac);
						memset(aircondition_status_hotCold, 0x0, DEVICE_STATUS_LEN);
						strcpy(aircondition_status_hotCold, "hot");					
					}
					else if(strcmp(para, "cold") == 0)
					{
						DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_COLD, NULL, focusMac);
						memset(aircondition_status_hotCold, 0x0, DEVICE_STATUS_LEN);
						strcpy(aircondition_status_hotCold, "cold");		
					}
					else{
						printf("error input:%s\n",para);
					}
					break;
				case 4:
					printf("Please input temp value: \n");
					scanf("%s", para);
					getchar();
					DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_TEMP, para, focusMac);
					memset(aircondition_status_temp, 0x0, DEVICE_STATUS_LEN);
					strcpy(aircondition_status_temp, para);	
					break;
				case 5:
					printf("Please input wind value(low/middle/high): \n");
					scanf("%s", para);
					getchar();
					DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_WIND, para, focusMac);
					memset(aircondition_status_wind, 0x0, DEVICE_STATUS_LEN);
					strcpy(aircondition_status_wind, para);
					break;
				case 6:
					DoDeviceNotice(alarmSockFd, &msg, name, focusMac);
					break;
				default:
					break;
			}
		}

SECOND_MENU_REFRIGERATOR:
		sleep(1);
		memset(&msg, 0x0, sizeof(MsgStruct));
		DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_REFRIGERATOR, MSG_CUSTOM2_GET_DEVICE_REF_FRESH, focusMac);
		memset(&msg, 0x0, sizeof(MsgStruct));
		DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_REFRIGERATOR, MSG_CUSTOM2_GET_DEVICE_REF_FREEZ, focusMac);
		while (1) {
			printf("**************************REFRIGERATOR***************************\n");
			printf("*   0.quit	1.return	2.fresh:%s	3.freeze:%s	4.Notice	5.Video\n",\
					refrigerator_status_fresh, 
			 		refrigerator_status_freez
			);
			printf("*****************************************************\n");
			memset(para, 0x00, 16);
			memset(&msg, 0x0, sizeof(MsgStruct));
			printf("Please choose:");
			scanf("%d", &cmd);
			getchar();
			switch (cmd) {
				case 0:
					close(userSockFd);
					close(deviceSockFd);
					close(alarmSockFd);
					close(mediaSockFd);
            		exit(0);
				case 1:
					goto FIRST_MENU;
					break;
				case 2:
					printf("Please input fresh temp value: \n");
					scanf("%s", para);
					getchar();
					DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_REFRIGERATOR, MSG_CUSTOM2_SET_DEVICE_REF_FRESH, para, focusMac);
					memset(refrigerator_status_fresh, 0x0, DEVICE_STATUS_LEN);
					strcpy(refrigerator_status_fresh, para);
					break;
				case 3:
					printf("Please input freeze temp value: \n");
					scanf("%s", para);
					getchar();
					DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_REFRIGERATOR, MSG_CUSTOM2_SET_DEVICE_REF_FREEZ, para, focusMac);
					memset(refrigerator_status_freez, 0x0, DEVICE_STATUS_LEN);
					strcpy(refrigerator_status_freez, para);
					break;
				case 4:
					memset(&msg, 0x0, sizeof(MsgStruct));
					DoDeviceNotice(alarmSockFd, &msg, name, focusMac);
					break;
				case 5:
					RequestVideoUrl(mediaSockFd, url);
					char exec[128] = {0};
					sprintf(exec, "vlc %s", url);
					printf("start vlc play : %s \n", exec);
					system(exec);
					break;
				default:
					break;
			}
		}
	}
	return 0;
}
