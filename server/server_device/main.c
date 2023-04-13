#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include "../include/message.h"

#define DATABASE "../database/smarthome_server.db"
#define MSGTYPE_ADD_DEVICE_D2S 1
#define MSGTYPE_ADD_DEVICE_S2D 2
#define MSGTYPE_STATUS_BASE_D2S 10
#define MSGTYPE_STATUS_BASE_S2D 1000

// #define DEBUG

struct MsgQueue
{
	long type;
	char mtext[MSG_DATA_MAX_LEN+MSG_HEADER_STABLE_LEN];
};

struct DeviceCnt
{
	int deviceCnt;
    pthread_mutex_t mutex;
};
static int shmid;
static int msgid;
static char macAddr[32];
static struct DeviceCnt *shareMemory;

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

static int CheckPattern(const char* values)
{
	for (int i = 0; values[i]; i++)
	{
		if (values[i] == '_') continue;
		if (values[i] >= '0' && values[i] <= '9') continue;
		if (values[i] >= 'a' && values[i] <= 'z') continue;
		if (values[i] >= 'A' && values[i] <= 'Z') continue;
		return -1;
	}
	return 0;
}

int GetLoginStatus_Callback(void* arg,int f_num,char** f_value,char** f_name)
{
	char *status = (char *)arg;
	printf("GetLoginStatus_Callback function called\n");
    for (int i = 0; i < f_num; i++)
	{
    	printf("%s = %s\n", f_name[i], f_value[i] ? f_value[i] : "NULL");
		if((strcmp(f_name[i], "loginstatus") == 0) || (strcmp(f_name[i], "LOGINSTATUS") == 0))
		{
			if(f_value[i])
			{
				strcpy(status, f_value[i]);
				return 0;
			}
		}
    }
	return 0;
}

int GetDeviceSocket_Callback(void* arg,int f_num,char** f_value,char** f_name)
{
	int *sockfd = (int *)arg;
	printf("GetDeviceSocket_Callback function called\n");
    for (int i = 0; i < f_num; i++)
	{
    	printf("%s = %s\n", f_name[i], f_value[i] ? f_value[i] : "NULL");
		if((strcmp(f_name[i], "sockfd") == 0) || (strcmp(f_name[i], "SOCKFD") == 0))
		{
			if(f_value[i])
			{
				*sockfd = atoi(f_value[i]);
				printf("device login, sockfd = %d.\n", *sockfd);
				return 0;
			}
		}
    }
	return 0;
}

int GetLoginStatus(const char* name, sqlite3 *db)
{
#if 1
	return 0;
#else
	char status[10] = "";
	char *errmsg = NULL;
	char sql[256] = {0};
	//查询user表name对应的登录状态，已登陆返回0，为登陆返回-1
	sprintf(sql, "select * from user where name = '%s';", name);
	printf("%s\n", sql);
	int ret = sqlite3_exec(db, sql, GetLoginStatus_Callback, (void *)&status, &errmsg);
	if (strcmp(status, "yes") != 0) return -1;
	return 0;
#endif
}

//Todo.每查到一条记录便会进入该回调函数一次
// 得到查询结果，并讲查询结果存放到data区域里面
static int deviceNum = 0;
int Devicelist_Callback(void* arg,int f_num,char** f_value,char** f_name)
{
	MsgStruct *msg = (MsgStruct *)arg;

	for(int i = 0; i < f_num; i++)
	{
    	printf("%s = %s\n", f_name[i], f_value[i] ? f_value[i] : "NULL");
		if((strcmp(f_name[i], "macaddr") == 0) || (strcmp(f_name[i], "MACADDR") == 0))
		{
			if(f_value[i])
			{
				printf("find a macaddr!\n");
				//23为macaddr固定长度
				strcpy(msg->data + deviceNum * 23, f_value[i]);
			}
			else
			{
				printf("macaddr is empty!\n");
				return 0;
			}
		}
    }
	deviceNum++;
	return 0;
}

//=========================Client->Server===========================
void QueryDeviceForClient(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	printf("DoQueryDevice\n");
	int curSeq = msg->seq_num;
	char *errmsg = NULL;
	char sql[256] = {0};
	char name[128] = {0};

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
	if(msg->protocol_type != MSG_PROTOCOL_C2S)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
		return;		
	}
	if(msg->frag_flag != MSG_FLAG_FRAG_NO)
	{
		printf("frag_flag error:0x%x.\n", msg->frag_flag);
		return;		
	}
	if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
	{
		printf("header_chk error:0x%x.\n", msg->header_chk);
		return;		
	}
	
	memcpy(name, msg->data, msg->custom1);

	memset(msg, 0x0, sizeof(MsgStruct));
	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_S2C;
	msg->data_type = MSG_DATA_QUERY_DEVICE;
	msg->seq_num = curSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->total_len = MSG_HEADER_STABLE_LEN*4;
	msg->source_addr = 0x7F000001;
	msg->target_addr = 0x7F000001;
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	
	if(CheckPattern(name) < 0)
	{
		printf("Name pattern:alphabet, number, underscore");
		msg->custom1 = MSG_CUSTOM1_QUERY_DEVICE_FAILURE;
		msg->custom2 = MSG_CUSTOM2_QUERY_DEVICE_FAILURE_ACCOUNT;
		if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
		{
			perror("fail to send.\n");
		}
		return;
	}

	if(GetLoginStatus(name, db) != 0)
	{
		printf("username:%s don't login.\n", name);
		msg->custom1 = MSG_CUSTOM1_QUERY_DEVICE_FAILURE;
		msg->custom2 = MSG_CUSTOM2_QUERY_DEVICE_FAILURE_ACCOUNT;
		if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
		{
			perror("fail to send.\n");
		}
		return;
	}

	//查询数据库
	sprintf(sql, "select * from device where username = '%s';", name);
	printf("%s\n", sql);
	int ret = sqlite3_exec(db, sql, Devicelist_Callback, (void *)msg, &errmsg);
	if(ret != SQLITE_OK)
	{
		printf("%d,%s\n", ret, errmsg);
		msg->custom1 = MSG_CUSTOM1_QUERY_DEVICE_FAILURE;
		msg->custom2 = MSG_CUSTOM2_QUERY_DEVICE_FAILURE_DATABASE;
	}
	else
	{
		printf("Query device list ok!\n");
		msg->custom1 = MSG_CUSTOM1_QUERY_DEVICE_SUCCESS;
		msg->custom2 = deviceNum;
	}
	sleep(1);

	if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		deviceNum = 0;
		return;
	}
	printf("send ok!\n");
	deviceNum = 0;
	return;
}

void AddDeviceForClient(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	//1.检查参数
	printf("ClientAddDeviceToServer\n");
	int curSeq = msg->seq_num;
	struct MsgQueue news;
	int newsType = 0;
	char sql[256] = {0};
	char mac[32] = {0};
	char name[32] = {0};
	char deviceName[32] = "device";
	char **dbResult = NULL; 
    int nRow = 0, nColumn = 0;
	char *errmsg = NULL;

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
	if(msg->protocol_type != MSG_PROTOCOL_C2S)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
		return;		
	}
	if(msg->frag_flag != MSG_FLAG_FRAG_NO)
	{
		printf("frag_flag error:0x%x.\n", msg->frag_flag);
		return;		
	}
	if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
	{
		printf("header_chk error:0x%x.\n", msg->header_chk);
		return;		
	}

	//2.从接收到客户端的数据包中获取name
	strncpy(name, msg->data, msg->custom1);

	//3.等待设备端发送消息给服务器（AddDeviceForDevice函数）
	printf("Waiting for divice...\n");
	memset(&news, 0, sizeof(struct MsgQueue));
	memset(msg, 0x0, sizeof(MsgStruct));
	newsType = MSGTYPE_ADD_DEVICE_D2S;
	if (msgrcv(msgid, &news, sizeof(struct MsgQueue), newsType, 0) < 0)
	{
		perror("msgrcv");
		msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_FAILURE;
		msg->custom2 = MSG_CUSTOM2_ADD_DEVICE_FAILURE_CONNECT;
	}
	else
	{
		//4.获取设备MAC地址（设备端发送过来的）
		strcpy(mac, news.mtext);
		//5.作为一条记录，插入数据库device表单
		sprintf(sql, "select * from device where macaddr = '%s';", mac);
		printf("%s\n", sql);
		int ret = sqlite3_get_table(db, sql, &dbResult, &nRow, &nColumn, &errmsg);
		if(ret != SQLITE_OK)
		{
			printf("database failed:%d,%s\n", ret, errmsg);
			msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_FAILURE;
			msg->custom2 = MSG_CUSTOM2_ADD_DEVICE_FAILURE_DATABASE;
			memset(&news, 0, sizeof(struct MsgQueue));
			news.type = MSGTYPE_ADD_DEVICE_S2D;
			news.mtext[0] = MSG_CUSTOM2_ADD_DEVICE_FAILURE_DATABASE;
			if (msgsnd(msgid, &news, sizeof(struct MsgQueue), 0) < 0)
			{
				perror("msgsnd");
			}
		}
		else if (nRow == 0)
		{
			pthread_mutex_lock(&shareMemory->mutex);
			sprintf(sql, "insert or replace into device values('%s', '%s%d', '%s', null);", name, deviceName, shareMemory->deviceCnt + 1, mac);
			printf("%s\n", sql);
			ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
			if (ret != SQLITE_OK)
			{
				printf("database failed:%d,%s\n", ret, errmsg);
				msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_FAILURE;
				msg->custom2 = MSG_CUSTOM2_ADD_DEVICE_FAILURE_DATABASE;
				memset(&news, 0, sizeof(struct MsgQueue));
				news.type = MSGTYPE_ADD_DEVICE_S2D;
				news.mtext[0] = MSG_CUSTOM2_ADD_DEVICE_FAILURE_DATABASE;
				if (msgsnd(msgid, &news, sizeof(struct MsgQueue), 0) < 0)
				{
					perror("msgsnd");
				}
			}
			else
			{
				printf("database success\n");
				msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_SUCCESS;
				strcpy(msg->data, news.mtext);

				shareMemory->deviceCnt += 1;
				memset(&news, 0, sizeof(struct MsgQueue));
				news.type = MSGTYPE_ADD_DEVICE_S2D;
				news.mtext[0] = MSG_CUSTOM1_ADD_DEVICE_SUCCESS;
				if (msgsnd(msgid, &news, sizeof(struct MsgQueue), 0) < 0)
				{
					perror("msgsnd");
				}
			}
			pthread_mutex_unlock(&shareMemory->mutex);
		}
		else
		{
			printf("mac already add.\n");
			msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_FAILURE;
			msg->custom2 = MSG_CUSTOM2_ADD_DEVICE_FAILURE_ALREADY;
			memset(&news, 0, sizeof(struct MsgQueue));
			news.type = MSGTYPE_ADD_DEVICE_S2D;
			news.mtext[0] = MSG_CUSTOM2_ADD_DEVICE_FAILURE_ALREADY;
			if (msgsnd(msgid, &news, sizeof(struct MsgQueue), 0) < 0)
			{
				perror("msgsnd");
			}
		}
	}
	//6.组装数据包，返回给客户端
	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_S2C;
	msg->data_type = MSG_DATA_ADD_DEVICE;
	msg->seq_num = curSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->total_len = MSG_HEADER_STABLE_LEN*4;
	msg->source_addr = 0x7F000001;
	msg->target_addr = 0x7F000001;
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return;
	}
	printf("send ok!\n");
	return;
}

//Todo
void GetDeviceStatusForClient(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	//1.检查参数
	printf("GetDeviceStatusForClient\n");
	// int curSeq = msg->seq_num;
	struct MsgQueue news;
	int newsType = 0;
	int sockfd = -1;
	char sql[256] = {0};
	char mac[32] = {0};
	char *errmsg = NULL;

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
	if(msg->protocol_type != MSG_PROTOCOL_C2S)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
		return;		
	}
	if(msg->frag_flag != MSG_FLAG_FRAG_NO)
	{
		printf("frag_flag error:0x%x.\n", msg->frag_flag);
		return;		
	}
	if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
	{
		printf("header_chk error:0x%x.\n", msg->header_chk);
		return;		
	}
	//2.从接收到客户端的数据包中获取设备mac
	strcpy(mac,msg->data);
	//3.检查该mac对应的设备是否在线

	//4.向该设备套接字发送读取设备状态消息
	sprintf(sql, "select * from device where macaddr = '%s';", mac);
	printf("%s\n", sql);
	int ret = sqlite3_exec(db, sql, GetDeviceSocket_Callback, (void *)&sockfd, &errmsg);
	if (ret != SQLITE_OK)
	{
		printf("database failed:%d,%s\n", ret, errmsg);
		return;
	}
	if (sockfd == -1)
	{
		printf("sockfd failed:%d\n", sockfd);
		return;		
	}

	memset(&news, 0, sizeof(struct MsgQueue));
	news.type = MSGTYPE_STATUS_BASE_S2D+sockfd;
	memcpy(news.mtext, msg, sizeof(MsgStruct));
	if (msgsnd(msgid, &news, sizeof(struct MsgQueue), 0) < 0)
	{
		perror("msgsnd");
		return;
	}

	//5.设备端返回设备状态
	memset(&news, 0x0, sizeof(struct MsgQueue));
	memset(msg, 0x0, sizeof(MsgStruct));
	newsType = MSGTYPE_STATUS_BASE_D2S+sockfd;
	printf("Waiting for device status...\n");
	if (msgrcv(msgid, &news, sizeof(struct MsgQueue), newsType, 0) < 0)
	{
		perror("msgsnd");
		return;
	}
	memcpy(msg, news.mtext, sizeof(MsgStruct));
	printf("Get device status success.\n");
	PrintMsg(msg);
	//6.组装数据包，返回给客户端
	msg->protocol_type = MSG_PROTOCOL_S2C;
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return;
	}
	printf("server device send to client ok!\n");
	return;
}

void SetDeviceStatusForClient(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	//1.检查参数
	printf("SetDeviceStatusForClient\n");
	// int curSeq = msg->seq_num;
	struct MsgQueue news;
	int newsType = 0;
	int sockfd = -1;
	char sql[256] = {0};
	char mac[32] = {0};
	char *errmsg = NULL;

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
	if(msg->protocol_type != MSG_PROTOCOL_C2S)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
		return;		
	}
	if(msg->frag_flag != MSG_FLAG_FRAG_NO)
	{
		printf("frag_flag error:0x%x.\n", msg->frag_flag);
		return;		
	}
	if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
	{
		printf("header_chk error:0x%x.\n", msg->header_chk);
		return;		
	}
	//2.从接收到客户端的数据包中获取设备mac
	memcpy(mac,msg->data,23);
	//3.检查该mac对应的设备是否在线

	//4.向该设备套接字发送设置设备状态消息
	sprintf(sql, "select * from device where macaddr = '%s';", mac);
	printf("%s\n", sql);
	int ret = sqlite3_exec(db, sql, GetDeviceSocket_Callback, (void *)&sockfd, &errmsg);
	if (ret != SQLITE_OK)
	{
		printf("database failed:%d,%s\n", ret, errmsg);
		return;
	}
	if (sockfd == -1)
	{
		printf("sockfd failed:%d\n", sockfd);
		return;		
	}

	memset(&news, 0, sizeof(struct MsgQueue));
	news.type = MSGTYPE_STATUS_BASE_S2D+sockfd;
	memcpy(news.mtext, msg, sizeof(MsgStruct));
	if (msgsnd(msgid, &news, sizeof(struct MsgQueue), 0) < 0)
	{
		perror("msgsnd");
		return;
	}

	//5.设备端返回设备状态
	memset(&news, 0x0, sizeof(struct MsgQueue));
	memset(msg, 0x0, sizeof(MsgStruct));
	newsType = MSGTYPE_STATUS_BASE_D2S+sockfd;
	printf("Waiting for device status...\n");
	if (msgrcv(msgid, &news, sizeof(struct MsgQueue), newsType, 0) < 0)
	{
		perror("msgsnd");
		return;
	}
	memcpy(msg, news.mtext, sizeof(MsgStruct));
	printf("Set device status success.\n");
	PrintMsg(msg);
	//6.组装数据包，返回给客户端
	msg->protocol_type = MSG_PROTOCOL_S2C;
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return;
	}
	printf("server device send to client ok!\n");
	return;
}

void NameDeviceForClient(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	//1.检查参数
	printf("ClientNameDeviceToServer\n");
	int curSeq = msg->seq_num;
	char sql[256] = {0};
	char mac[32] = {0};
	char name[32] = {0};
	char deviceName[32] = {0};
	char *errmsg = NULL;

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
	if(msg->protocol_type != MSG_PROTOCOL_C2S)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
		return;		
	}
	if(msg->frag_flag != MSG_FLAG_FRAG_NO)
	{
		printf("frag_flag error:0x%x.\n", msg->frag_flag);
		return;		
	}
	if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
	{
		printf("header_chk error:0x%x.\n", msg->header_chk);
		return;		
	}
	//2.从接收到客户端的数据包中获取用户名、设备MAC地址、设备名字
	strncpy(name, msg->data, msg->custom1);
	strncpy(mac, msg->data + msg->custom1, 23);
	strncpy(deviceName, msg->data + msg->custom1 + 23, msg->custom2);

	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_S2C;
	msg->data_type = MSG_DATA_NAME_DEVICE;
	msg->seq_num = curSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->total_len = MSG_HEADER_STABLE_LEN*4;
	msg->source_addr = 0x7F000001;
	msg->target_addr = 0x7F000001;
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);

	if(CheckPattern(deviceName) < 0)
	{
		printf("DeviceName pattern:alphabet, number, underscore");
		msg->custom1 = MSG_CUSTOM1_NAME_DEVICE_FAILURE;
		msg->custom2 = MSG_CUSTOM2_NAME_DEVICE_FAILURE_FORMAT;
		if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
		{
			perror("fail to send.\n");
		}
		return;
	}
	//3.作为一条记录，更新数据库device表单
	sprintf(sql, "update device set devicename = '%s' where macaddr = '%s';", deviceName, mac);
	printf("%s\n", sql);
	int ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if(ret != SQLITE_OK)
	{
		printf("database failed:%d,%s\n", ret, errmsg);
		msg->custom1 = MSG_CUSTOM1_NAME_DEVICE_FAILURE;
		msg->custom2 = MSG_CUSTOM2_NAME_DEVICE_FAILURE_DATABASE;
	}
	else
	{
		msg->custom1 = MSG_CUSTOM1_NAME_DEVICE_SUCCESS;
	}
	//4.组装数据包，返回给客户端
	if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return;
	}
	printf("send ok!\n");
	return;
}

void DoClient(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	switch(msg->data_type)
	{
		//Query Device
		case MSG_DATA_QUERY_DEVICE:
			QueryDeviceForClient(acceptfd, msg, db);
			break;
		//Add Device
		case MSG_DATA_ADD_DEVICE:
			AddDeviceForClient(acceptfd, msg, db);
			break;
		//Set Device Status
		case MSG_DATA_SET_DEVICE_STATUS:
			SetDeviceStatusForClient(acceptfd, msg, db);
			break;
		//Get Device Status
		case MSG_DATA_GET_DEVICE_STATUS:
			GetDeviceStatusForClient(acceptfd, msg, db);
			break;
		//Name Device
		case MSG_DATA_NAME_DEVICE:
			NameDeviceForClient(acceptfd, msg, db);
			break;
		default:
			printf("Invalid data msg.\n");
			break;
	}
}

//=========================Device->Server===========================
//Todo
void AddDeviceForDevice(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	//1.检查参数
	printf("DeviceAddDeviceToServer\n");
	int curSeq = msg->seq_num;
	char mac[32] = {0};
	char sql[256] = {0};
	char *errmsg = NULL;
	struct MsgQueue news;
	int newsType = 0;
	
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
	if(msg->protocol_type != MSG_PROTOCOL_D2S)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
		return;		
	}
	if(msg->frag_flag != MSG_FLAG_FRAG_NO)
	{
		printf("frag_flag error:0x%x.\n", msg->frag_flag);
		return;		
	}
	if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
	{
		printf("header_chk error:0x%x.\n", msg->header_chk);
		return;		
	}
	//2.从接收到设备端的数据包中获取mac
	strcpy(mac, msg->data);
	//3.通知AddDeviceForClient设备的mac准备好了
	memset(&news, 0, sizeof(struct MsgQueue));
	news.type = MSGTYPE_ADD_DEVICE_D2S;
	strcpy(news.mtext, mac);
	printf("Add mac: %s\n", news.mtext);
	if (msgsnd(msgid, &news, sizeof(struct MsgQueue), 0) < 0)
	{
		perror("msgsnd");
		msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_FAILURE;
		msg->custom2 = MSG_CUSTOM2_ADD_DEVICE_FAILURE_CONNECT;
	}
	memset(&news, 0, sizeof(struct MsgQueue));
	newsType = MSGTYPE_ADD_DEVICE_S2D;
	if (msgrcv(msgid, &news, sizeof(struct MsgQueue), newsType, 0) < 0)
	{
		perror("msgrcv");
		msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_FAILURE;
		msg->custom2 = MSG_CUSTOM2_ADD_DEVICE_FAILURE_CONNECT;
	}
	else
	{
		switch (news.mtext[0])
		{
			case MSG_CUSTOM1_ADD_DEVICE_SUCCESS: 
				{
					msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_SUCCESS;
				} break;
			case MSG_CUSTOM2_ADD_DEVICE_FAILURE_ALREADY:
				{
					msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_FAILURE;
					msg->custom2 = MSG_CUSTOM2_ADD_DEVICE_FAILURE_ALREADY;
				} break;
			case MSG_CUSTOM2_ADD_DEVICE_FAILURE_DATABASE:
				{
					msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_FAILURE;
					msg->custom2 = MSG_CUSTOM2_ADD_DEVICE_FAILURE_DATABASE;
				} break;
			default: break;
		}
	}
	//4.组装数据包，返回给设备端
	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_S2D;
	msg->data_type = MSG_DATA_ADD_DEVICE;
	msg->seq_num = curSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->total_len = MSG_HEADER_STABLE_LEN*4;
	msg->source_addr = 0x7F000001;
	msg->target_addr = 0x7F000001;
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);

	//5.保存套接字acceptfd，在客户端发起读取设备状态、设置设备状态时使用。
	sprintf(sql, "update device set sockfd = '%d' where macaddr = '%s';", acceptfd, mac);
	printf("%s\n", sql);
	int ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if(ret != SQLITE_OK)
	{
		printf("database failed:%d,%s\n", ret, errmsg);
		msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_FAILURE;
		msg->custom2 = MSG_CUSTOM2_ADD_DEVICE_FAILURE_DATABASE;
	}
	else
	{
		msg->custom1 = MSG_CUSTOM1_ADD_DEVICE_SUCCESS;
		strcpy(macAddr, mac);
	}
	if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return;
	}
	printf("send ok!\n");
	return;
}

//Todo
void ConnectServerForDevice(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	//1.检查参数
	printf("DeviceConnectDeviceToServer\n");
	int curSeq = msg->seq_num;
	char mac[32] = {0};
	char sql[256] = {0};
	char *errmsg = NULL;
	char **dbResult = NULL; 
    int nRow = 0, nColumn = 0;
	
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
	if(msg->protocol_type != MSG_PROTOCOL_D2S)
	{
		printf("protocol_type error:0x%x.\n", msg->protocol_type);
		return;		
	}
	if(msg->frag_flag != MSG_FLAG_FRAG_NO)
	{
		printf("frag_flag error:0x%x.\n", msg->frag_flag);
		return;		
	}
	if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
	{
		printf("header_chk error:0x%x.\n", msg->header_chk);
		return;		
	}
	//2.从接收到设备端的数据包中获取mac
	strcpy(mac, msg->data);
	//3.检查数据库中是否有该mac地址
	sprintf(sql, "select * from device where macaddr = '%s';", mac);
	printf("%s\n", sql);
	int ret = sqlite3_get_table(db, sql, &dbResult, &nRow, &nColumn, &errmsg);
	if(ret != SQLITE_OK)
	{
		printf("database failed:%d,%s\n", ret, errmsg);
		msg->custom1 = MSG_CUSTOM1_DEVICE_CONNECTION_FAILURE;
		msg->custom2 = MSG_CUSTOM2_DEVICE_CONNECTION_FAILURE_NET;
	}
	if (nRow == 0)
	{
		msg->custom1 = MSG_CUSTOM1_DEVICE_CONNECTION_FAILURE;
		msg->custom2 = MSG_CUSTOM2_DEVICE_CONNECTION_FAILURE_NOTADD;
	}
	//4.组装数据包，返回给设备端
	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_S2D;
	msg->data_type = MSG_DATA_DEVICE_CONNECTION;
	msg->seq_num = curSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->total_len = MSG_HEADER_STABLE_LEN*4;
	msg->source_addr = 0x7F000001;
	msg->target_addr = 0x7F000001;
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	//5.保存套接字acceptfd，在客户端发起读取设备状态、设置设备状态时使用。
	sprintf(sql, "update device set sockfd = '%d' where macaddr = '%s';", acceptfd, mac);
	printf("%s\n", sql);
	ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK)
	{
		printf("database failed:%d,%s\n", ret, errmsg);
		msg->custom1 = MSG_CUSTOM1_DEVICE_CONNECTION_FAILURE;
		msg->custom2 = MSG_CUSTOM2_DEVICE_CONNECTION_FAILURE_MAC;
	}
	else
	{
		msg->custom1 = MSG_CUSTOM1_DEVICE_CONNECTION_SUCCESS;
		strcpy(macAddr, mac);
	}
	if (send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return;
	}
	printf("send ok!\n");
	return;
}

void GetDeviceResult(int acceptfd, MsgStruct *msg)
{
	printf("GetDeviceResult.\n");
	struct MsgQueue news;

	memset(&news, 0, sizeof(struct MsgQueue));
	news.type = MSGTYPE_STATUS_BASE_D2S+acceptfd;
	memcpy(news.mtext, msg, msg->total_len);

	if (msgsnd(msgid, &news, sizeof(struct MsgQueue), 0) < 0)
	{
		perror("msgsnd");
		return;
	}

	printf("Device return result:%d\n", acceptfd);
	return;
}

void DoDevice(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	switch(msg->data_type)
	{
		//Add Device
		case MSG_DATA_ADD_DEVICE:
			AddDeviceForDevice(acceptfd, msg, db);
			break;
		//Device Connection已被添加设备连接服务器
		case MSG_DATA_DEVICE_CONNECTION:
			ConnectServerForDevice(acceptfd, msg, db);
			break;
		case MSG_DATA_SET_DEVICE_STATUS:
		case MSG_DATA_GET_DEVICE_STATUS:
			GetDeviceResult(acceptfd, msg);
			break;
		default:
			printf("Invalid data msg.\n");
			break;
	}
}

void DeviceLogOut(sqlite3 *db)
{
	char sql[256] = {0};
	char *errmsg = NULL;
	sprintf(sql, "update device set sockfd = null where macaddr = '%s';", macAddr);
	printf("%s\n", sql);
	int ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if(ret != SQLITE_OK)
	{
		printf("database failed:%d,%s\n", ret, errmsg);
		return;
	}
	printf("device logout.\n");
	return;
}

static void* DealwithStatus(void *arg)
{
	printf("%s:IN\r\n", __FUNCTION__);
	int deviceSockFd = *(int *)arg;
	MsgStruct msg;
	struct MsgQueue news;
	int newsType = 0;
	while (1)
	{
		//1.客户端发来读取/设置设备状态
		memset(&news, 0x0, sizeof(struct MsgQueue));
		memset(&msg, 0x0, sizeof(MsgStruct));

		newsType = MSGTYPE_STATUS_BASE_S2D+deviceSockFd;
		printf("Waiting for device set/get status from client...\n");
		if (msgrcv(msgid, &news, sizeof(struct MsgQueue), newsType, 0) < 0)
		{
			perror("msgrcv");
			continue;
		}
		printf("Set/Get device status success:%d.\n", deviceSockFd);
		memcpy(&msg, news.mtext, sizeof(MsgStruct));

		//2.组装数据包，发送给设备端
		msg.protocol_type = MSG_PROTOCOL_S2D;
		msg.header_chk = GetChkSum((char*)(&msg), MSG_HEADER_STABLE_LEN);
		if (send(deviceSockFd, &msg, sizeof(MsgStruct), 0) < 0)
		{
			perror("fail to send.\n");
			continue;
		}
		printf("server device send to device ok!\n");
	}
	
	printf("thead exit.\n");
	close(deviceSockFd);

	return NULL;
}

//一个新连接，可能是客户端，也可能是设备端
int DoChild(int acceptfd, sqlite3 *db)
{
	int isDeviceFlag = 0;
    pthread_t threadStatus_t;
	MsgStruct msg;
	while(recv(acceptfd, &msg, sizeof(msg), 0) > 0)
	{
		if(msg.protocol_type == MSG_PROTOCOL_C2S)
		{
			printf("Server Device:It's a client connection.\n");
			DoClient(acceptfd, &msg, db);
		}
		else if(msg.protocol_type == MSG_PROTOCOL_D2S)
		{
			printf("Server Device::It's a device connection.\n");
			DoDevice(acceptfd, &msg, db);
			if(isDeviceFlag == 0)
			{
				pthread_create(&threadStatus_t, NULL, DealwithStatus, &acceptfd);
			}
			isDeviceFlag = 1;
		}
		else
		{
			printf("protocol_type error:0x%x.\n", msg.protocol_type);			
			continue;
		}
		memset(&msg, 0x0, sizeof(MsgStruct));
	}
	if (isDeviceFlag)
	{
		DeviceLogOut(db);
	}
	printf("client exit.\n");
	close(acceptfd);
	exit(0);

	return 0;
}

void HandlerChildSig(int sig)
{
	printf("Server_device HandlerChildSig IN.\n");
	if(sig == SIGCHLD)
	{
		waitpid(-1,NULL,WNOHANG);
	}		
}

void FreeResource(int sig)
{
	printf("FreeResource.\n");
	if (sig == SIGINT)
	{
		pthread_mutex_destroy(&shareMemory->mutex);
    	shmdt(shareMemory);
    	shmctl(shmid, IPC_RMID, NULL);
		msgctl(msgid, IPC_RMID, 0);
		exit(0);
	}
	return;	
}

// ./server_device
int main(int argc, const char *argv[])
{
	int sockfd = -1;
	int newfd = -1;

	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	sqlite3 *db;
	char *errmsg;
	pid_t pid;

    printf("Server Device :IN\n");
	//打开数据库
	if(sqlite3_open(DATABASE, &db) != SQLITE_OK)
	{
		printf("%s\n", sqlite3_errmsg(db));
		return -1;
	}
	printf("open DATABASE success.\n");

	if(sqlite3_exec(db,"create table device(username text, devicename text, macaddr text, sockfd text);",NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
	}	

	if((sockfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}

	int optval = 1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(SERVER_MASTER_IP);
	sin.sin_port = htons(SERVER_DEVICE_PORT);

	if(bind(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		perror("fail to bind.\n");
		return -1;
	}

	// 将套接字设为监听模式
	if(listen(sockfd, 5) < 0)
	{
		perror("fail to listen.\n");
		return -1;
	}
	//创建共享内存记录设备总数
    key_t key = ftok(".", 2021);
    if ((shmid = shmget(key, sizeof(struct DeviceCnt), IPC_CREAT | 0666)) < 0)
	{
        perror("shmget");
        return -1;
    }
    if ((shareMemory = (struct DeviceCnt *)shmat(shmid, NULL, 0)) < 0)
	{
        perror("shmat");
        return -1;
    }
	shareMemory->deviceCnt = 0;
	pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shareMemory->mutex, &attr);
	
	key = ftok(".", 2022);
	if ((msgid = msgget(key, IPC_CREAT | 0666)) < 0)
	{
		perror("msgget");
		return -1;
	}

	signal(SIGINT, FreeResource);
	while(1)
	{
		if((newfd = accept(sockfd, (struct sockaddr *)&sin, &len)) < 0)
		{
			perror("fail to accept");
			continue;
		}
		
		if((pid = fork()) < 0)
		{
			perror("fail to fork");
			continue;
		}
		else if(pid == 0)  // 子进程
		{
			//处理客户端具体的消息
			printf("Server Device:A new child connection.\n");
			close(sockfd);
			DoChild(newfd, db);
		}
		else  // 父进程,用来接受客户端的请求的
		{
			//close(newfd);
			signal(SIGCHLD, HandlerChildSig);
			
		}
	}

	return 0;
}