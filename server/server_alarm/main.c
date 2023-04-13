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
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stddef.h>
#include <sys/un.h>
#include <pthread.h>
#include <errno.h>
#include "../include/message.h"

#define CLIENT_FD_SIZE 10
#define CLIENT_FD_SHM   "ALARMSERVER_CLIENT_FD_SHM"

#define DATABASE "../database/smarthome_server.db"
// #define DEBUG

typedef struct ClientFd//客户端套接字
{
	char mac[32];
	int  acceptfd;
}ClientFd;
ClientFd* totalClientFd;

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
	for (int i = 0; i < strlen(values); i++)
	{
		if(values[i] >= '0' && values[i] <= '9' )
		{
			continue;
		}
		if(values[i] >= 'a' && values[i] <= 'z' )
		{
			continue;
		}	
		if(values[i] >= 'A' && values[i] <= 'Z' )
		{
			continue;
		}	
		if(values[i] == '_')
		{
			continue;
		}
		return -1;		
	}
	
	return 0;
}

//创建并打开共享内存
static void *CreateAndMapSharedObj(const char *const name, const size_t size)
{
    void *shmptr = NULL;
    int   shmdes = -1;
    char  shmName[128];

    snprintf(shmName,sizeof(shmName),"%s-%d",name,getuid());
    if ((shmdes = shm_open(shmName, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG)) == -1)
    {
        printf("opening shared memory '%s' failed, errno %d",shmName,errno);
        return NULL;
    }

    if (ftruncate(shmdes, size) == -1)
    {
        printf("resizing shared memory '%s' to %zu failed, errno %d",shmName, size, errno);
        shm_unlink(shmName);
        close(shmdes);
        return NULL;
    }

    if ((shmptr = mmap(NULL,size,PROT_WRITE | PROT_READ, MAP_SHARED,shmdes,0)) == (void *)(-1))
    {
        printf("mapping shared memory '%s' failed, errno %d",shmName,errno);
        shm_unlink(shmName);
        close(shmdes);
        return NULL;
    }

    (void)close(shmdes);
    return shmptr;
}

//Todo.获取登录状态
int GetLoginStatus(const char* name, sqlite3 *db)
{
	//查询user表name对应的登录状态
	return 0;
}

//Todo.每查到一条记录便会进入该回调函数一次
// 得到查询结果，并讲查询结果存放到data区域里面
static int alarmNoticeNum = 0;
int AlarmNotice_Callback(void* arg,int f_num,char** f_value,char** f_name)
{
	MsgStruct *msg = (MsgStruct *)arg;
	alarmNoticeNum++;
	int curNoticeLen = strlen(msg->data);
	printf("AlarmNotice_Callback=%d, curNoticeLen=%d\n", alarmNoticeNum, curNoticeLen);

	if (MSG_DATA_MAX_LEN < curNoticeLen)
	{
		printf("Data Full\n");
		memset(msg->data, 0x0, MSG_DATA_MAX_LEN);
	}

	for (int i = 0; i < f_num; i++)
	{
		// printf("len=%d, value=%s\n", strlen(msg->data), f_value[i]);
		strcat(msg->data, f_value[i]);			
	}
	return 0;
}

//=========================Client->Server===========================
void GetAlarmNoticeForClient(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	printf("GetAlarmNoticeForClient\n");
	int curSeq = msg->seq_num;
	char *errmsg = NULL;
	char sql[256] = {0};
	char name[128] = {0};
	char mac[32] = {0};

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
	strcpy(mac, msg->data+msg->custom1);
	if(CheckPattern(name) < 0)
	{
		printf("Name pattern:alphabet, number, underscore");
		return;
	}

	memset(msg, 0x0, sizeof(MsgStruct));
	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_S2C;
	msg->data_type = MSG_DATA_GET_ALARM_NOTICE;
	msg->seq_num = curSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->total_len = MSG_HEADER_STABLE_LEN*4;
	msg->source_addr = 0x7F000001;
	msg->target_addr = 0x7F000001;
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	if(GetLoginStatus(name,db) != 0)
	{
		printf("username:%s don't login.\n", name);
		msg->custom1 = MSG_CUSTOM1_GET_ALARM_NOTICE_FAILURE;
		msg->custom2 = MSG_CUSTOM2_GET_ALARM_NOTICE_ACCOUNT;
		if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
		{
			perror("fail to send.\n");
		}
		return;
	}

	//查询数据库
	sprintf(sql, "select * from notice where macaddr = '%s';", mac);
	printf("%s\n", sql);
	int ret = sqlite3_exec(db, sql, AlarmNotice_Callback, (void *)msg, &errmsg);
	if(ret != SQLITE_OK)
	{
		printf("%d,%s\n", ret, errmsg);
		msg->custom1 = MSG_CUSTOM1_GET_ALARM_NOTICE_FAILURE;
		msg->custom2 = MSG_CUSTOM2_GET_ALARM_NOTICE_DATABASE;
	}
	else
	{
		printf("Query alarm notice list ok!\n");
		sleep(1);
		msg->custom1 = MSG_CUSTOM1_GET_ALARM_NOTICE_SUCCESS;
		msg->custom2 = alarmNoticeNum;
	}

	PrintMsg(msg);
	if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return;
	}
	// printf("send ok!\n");

    //Todo.保存套接字acceptfd，在设备端发送告警时转发给客户端使用。
	//若新客户端，则Mac地址保存在Clientacceptfd中方便下次调用
	for(int i=0; i<CLIENT_FD_SIZE; i++)
	{
		if(strcmp(totalClientFd[i].mac, mac) == 0)
		{
			printf("Update client fd. No.%d\n",i);
			totalClientFd[i].acceptfd = acceptfd;
			break;
		}

		if(totalClientFd[i].acceptfd == 0)
		{
			totalClientFd[i].acceptfd = acceptfd;
			strcpy(totalClientFd[i].mac, mac);
			printf("Save client fd=%d. No.%d\n",acceptfd, i);
			break;
		}
	}
	return;
}

void  DeviceAlarmNotifyToClient(int acceptfd, MsgStruct *msg)
{
	//该函数由DeviceAlarmNotifyToServer调用
	//1.转发数据包，发送给客户端
	msg->protocol_type = MSG_PROTOCOL_S2C;
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
	PrintMsg(msg);
	if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	{
		perror("fail to send.\n");
		return;
	}
	printf("send ok!\n");
	//tips:这里并不需要客户端返回数据
	return;
}

void DoClient(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	switch(msg->data_type)
	{
		//Get Alarm Notice
		case MSG_DATA_GET_ALARM_NOTICE:
			GetAlarmNoticeForClient(acceptfd, msg, db);
			break;
		default:
			printf("Invalid data msg.\n");
			break;
	}
}
//=========================Device->Server===========================

//Todo
void DeviceAlarmNotifyToServer(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	char mac[32] = {0};
	int alarmcode;
	time_t timep;
    struct tm *p;
	char alarmtime[20] = {0};
	char *errmsg = NULL;
	char sql[256] = {0};
	//1.检查参数
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
	//2.从接收到设备端的数据包中获取设备mac、设备类型、告警类型
	strcpy(mac, msg->data);
	alarmcode = msg->custom2;
    //3.获取当前时间，并同mac地址、告警时间、告警类型作为一条记录插入到notice表中
    time(&timep);
    p = gmtime(&timep);
    sprintf(alarmtime, "%04d-%02d-%02d %02d:%02d:%02d", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
	sprintf(sql, "INSERT INTO notice VALUES ('%s', '%s', '%d')", mac, alarmtime, alarmcode);
	printf("%s\n", sql);
	int ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if(ret != SQLITE_OK)
	{
		printf("%d,%s\n", ret, errmsg);
	}
	else
	{
		printf("Inster into notice valuse ok!\n");
	}
	//4.向客户端套接字告警消息，调用DeviceAlarmNotifyToClient()函数
	for(int i=0; i<CLIENT_FD_SIZE; i++)
	{
		if(strcmp(totalClientFd[i].mac, mac) == 0)
		{
			printf("Find client fd=%d. No.%d\n",totalClientFd[i].acceptfd, i);
			DeviceAlarmNotifyToClient(totalClientFd[i].acceptfd, msg);
			return;
		}
	}

	return;
    //tips:这里并不需要向设备端返回数据
}

void DoDevice(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	switch(msg->data_type)
	{
		//Device Alarm Notify
		case MSG_DATA_DEVICE_ALARM_NOTIFY:
			DeviceAlarmNotifyToServer(acceptfd, msg, db);
			break;
		default:
			printf("Invalid data msg.\n");
			break;
	}
}

//一个新连接，可能是客户端，也可能是设备端
int DoChild(int acceptfd, sqlite3 *db)
{
	MsgStruct msg;
	memset(&msg, 0x0, sizeof(MsgStruct));
	while(recv(acceptfd, &msg, sizeof(msg), 0) > 0)
	{
		if(msg.protocol_type == MSG_PROTOCOL_C2S)
		{
			printf("Server Alarm:It's a client connection.\n");
			DoClient(acceptfd, &msg, db);
		}
		else if(msg.protocol_type == MSG_PROTOCOL_D2S)
		{
			printf("Server Alarm:It's a device connection.\n");
			DoDevice(acceptfd, &msg, db);
		}
		else
		{
			printf("protocol_type error:0x%x.\n", msg.protocol_type);			
			continue;
		}
		memset(&msg, 0x0, sizeof(MsgStruct));
	}

	printf("client exit.\n");
	close(acceptfd);
	exit(0);

	return 0;
}

void HandlerChildSig(int sig)
{
	printf("Server_alarm HandlerChildSig IN.\n");
	if(sig == SIGCHLD)
	{
		waitpid(-1,NULL,WNOHANG);
	}		
}

// ./server_alarm
int main(int argc, const char *argv[])
{
	int sockfd = -1;
	int newfd = -1;

	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	sqlite3 *db;
	char *errmsg;
	pid_t pid;

    printf("Server Alarm :IN\n");
	//打开数据库
	if(sqlite3_open(DATABASE, &db) != SQLITE_OK)
	{
		printf("%s\n", sqlite3_errmsg(db));
		return -1;
	}
	printf("open DATABASE success.\n");

    int ret = sqlite3_exec(db,"create table notice(macaddr text, alarmtime text, alarmtype text);",
                            NULL,NULL,&errmsg);
	if(ret != SQLITE_OK)
	{
		printf("%s\n",errmsg);
	}	

	if((sockfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}

	//创建共享内存
	totalClientFd = (ClientFd *)CreateAndMapSharedObj(CLIENT_FD_SHM, CLIENT_FD_SIZE*sizeof(ClientFd));
    if(NULL == totalClientFd)
    {
        printf("Unable to open and map shared mem object\r\n");
        return -1;
    }
    memset(totalClientFd, 0, CLIENT_FD_SIZE*sizeof(ClientFd));

	//创建套接字
	int optval = 1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(SERVER_MASTER_IP);
	sin.sin_port = htons(SERVER_ALARM_PORT);

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

	//循环
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
			printf("Server Alarm:A new child connection.\n");
			close(sockfd);
			DoChild(newfd, db);
		}
		else  // 父进程,用来接受客户端的请求的
		{
			close(newfd);
			signal(SIGCHLD, HandlerChildSig);
		}
	}

	return 0;
}