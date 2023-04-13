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
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <termios.h>
#include "../../include/message.h"

#include "video.h"

static char url[MSG_DATA_MAX_LEN] = {0};

#define DATABASE "../../database/smarthome_server.db"
#define DEBUG
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

static char macAddr[32];

static int CheckPattern(const char* values)
{
	return 0;
}

int GetLoginStatus_Callback(void* arg,int f_num,char** f_value,char** f_name)
{
	// char *status = (char *)arg;
	printf("GetLoginStatus_Callback function called\n");
	return 0;
}


int GetLoginStatus(const char* name, sqlite3 *db)
{
	return 0;
}

int GetDeviceStatus(const char *mac, sqlite3 *db)
{
	return 0;
}

// 得到查询结果
int GetProxyUrl_Callback(void* arg,int f_num,char** f_value,char** f_name)
{
	// MsgStruct *msg = (MsgStruct *)arg;
	printf("GetProxyUrl_Callback function called\n");
	return 0;
}

//=========================Client->Server===========================
void GetProxyUrlForClient(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	printf("GetProxyUrlForClient\n");
	int curSeq = msg->seq_num;
	// char *errmsg = NULL;
	// char sql[256] = {0};
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
	if(CheckPattern(name) < 0)
	{
		printf("Name pattern:alphabet, number, underscore");
		return;
	}
	strcpy(mac, msg->data+msg->custom1);

	memset(msg, 0x0, sizeof(MsgStruct));
	msg->version = MSG_VERSION;
	msg->header_len = MSG_HEADER_STABLE_LEN;
	msg->encrypt_type = MSG_ENCRYPT_NONE;
	msg->protocol_type = MSG_PROTOCOL_S2C;
	msg->data_type = MSG_DATA_GET_PROXY_URL;
	msg->seq_num = curSeq;
	msg->frag_flag = MSG_FLAG_FRAG_NO;
	msg->frag_offset = 0;
	msg->total_len = MSG_HEADER_STABLE_LEN*4;
	msg->source_addr = 0x7F000001;
	msg->target_addr = 0x7F000001;
	msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);

    int url_fd = open("./url", O_RDWR);
    if (url_fd == -1)
    {
        perror("fail to open\n");
        return;
    }
    if (read(url_fd, url, MSG_DATA_MAX_LEN) == -1)
    {
        perror("fail to read url\n");
        return;
    }
    close(url_fd);


	msg->total_len = MSG_HEADER_STABLE_LEN*4 + strlen(url);
 	printf("url %p : %s\n", url, url);
 	memcpy(msg->data, url, strlen(url));

	int nRet = send(acceptfd, msg, sizeof(MsgStruct), 0);
	if(nRet <= 0)
	{
		printf("send to client error \n");
	}
	// if(GetLoginStatus(name, db) != 0)
	// {
	// 	printf("username:%s don't login.\n", name);
	// 	msg->custom1 = MSG_CUSTOM1_GET_PROXY_URL_FAILURE;
	// 	msg->custom2 = MSG_CUSTOM2_GET_PROXY_URL_ACCOUNT;
	// 	if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	// 	{
	// 		perror("fail to send.\n");
	// 	}
	// 	return;
	// }

	// //查询数据库
	// sprintf(sql, "select * from url where macaddr = '%s';", mac);
	// printf("%s\n", sql);
	// int ret = sqlite3_exec(db, sql, GetProxyUrl_Callback, (void *)msg, &errmsg);
	// if(ret != SQLITE_OK)
	// {
	// 	printf("%d,%s\n", ret, errmsg);
	// 	msg->custom1 = MSG_CUSTOM1_GET_PROXY_URL_FAILURE;
	// 	msg->custom2 = MSG_CUSTOM2_GET_PROXY_URL_DATABASE;
	// }

	// sleep(1);
	// //数据库中不存在macaddr
	// if (strlen(msg->data) == 0) {
	// 	printf("macaddr error.\n");
	// 	msg->custom1 = MSG_CUSTOM1_GET_PROXY_URL_FAILURE;
	// 	msg->custom2 = MSG_CUSTOM2_GET_PROXY_URL_DATABASE;
	// }

	// if(send(acceptfd, msg, sizeof(MsgStruct), 0) < 0)
	// {
	// 	perror("fail to send.\n");
	// 	return;
	// }
	printf("send ok!\n");
	return;
}

void DoClient(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	switch(msg->data_type)
	{
		//Get Proxy Url
		case MSG_DATA_GET_PROXY_URL:
			GetProxyUrlForClient(acceptfd, msg, db);
			break;
		default:
			printf("Invalid data msg.\n");
			break;
	}
}
//=========================Device->Server===========================

void DeviceUrlNotifyToServer(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	//1.检查参数
	//Check Some Param
	//判断是启动代理还是停止代理
	printf("DeviceUrlNotifyToServer\n");
	if (msg->custom1 == MSG_CUSTOM1_DEVICE_URL_NOTICE_START)
	{
		//2.从接收到设备端的数据包中获取设备mac、设备URL地址

		//3.根据设备URL地址产生代理URL地址
		memcpy(url, msg->data, msg->total_len - MSG_HEADER_STABLE_LEN*4);
		printf("device url %p : %s \n", url, url);
		//4.检查设备是否被添加
	
		//5.启动代理服务器
		char proxyurl[MSG_DATA_MAX_LEN] = {0};
		StartVideo(url, proxyurl);

		printf("proxy url : %s \n", proxyurl);
		//6.数据库操作
		int url_fd = open("./url", O_RDWR);
		if (url_fd == -1)
		{
			perror("fail to open\n");
			return;
		}
		if (write(url_fd, proxyurl, MSG_DATA_MAX_LEN) == -1)
		{
			perror("fail to read url\n");
			return;
		}
		close(url_fd);
		printf("Get proxy url ok\n");
		//老设备，更新原有记录

		//新设备，增加一条记录
	}
	else if(msg->custom1 == MSG_CUSTOM1_DEVICE_URL_NOTICE_STOP)
	{
		//1.数据库操作
		//删掉对应记录
	}
	else
	{
		printf("ERROR: msg->custom1 = %d\n", msg->custom1);
		return;
	}

	//组装数据，返回设备端
	return;
}

void DoDevice(int acceptfd, MsgStruct *msg, sqlite3 *db)
{
	switch(msg->data_type)
	{
		//Device Url Notify
		case MSG_DATA_DEVICE_URL_NOTICE:
			DeviceUrlNotifyToServer(acceptfd, msg, db);
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
			printf("Server Media:It's a client connection.\n");
			DoClient(acceptfd, &msg, db);
		}
		else if(msg.protocol_type == MSG_PROTOCOL_D2S)
		{
			printf("Server Media:It's a device connection.\n");
			DoDevice(acceptfd, &msg, db);
		}
		else
		{
			printf("protocol_type error:0x%x.\n", msg.protocol_type);			
			continue;
		}
		memset(&msg, 0x0, sizeof(MsgStruct));
	}
	printf("child exit.\n");
	close(acceptfd);
	exit(0);

	return 0;
}

void HandlerChildSig(int sig)
{
	printf("Server_media HandlerChildSig IN.\n");
	if(sig == SIGCHLD)
	{
		waitpid(-1,NULL,WNOHANG);
	}		
}

// ./server_media
int main(int argc, const char *argv[])
{
	int sockfd = -1;
	int newfd = -1;

	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	pid_t pid;
	sqlite3 *db;
	// char *errmsg;

    printf("Server Media :IN\n");
	// //打开数据库
	// if(sqlite3_open(DATABASE, &db) != SQLITE_OK)
	// {
	// 	printf("%s\n", sqlite3_errmsg(db));
	// 	return -1;
	// }
	// printf("open DATABASE success.\n");

    // int ret = sqlite3_exec(db,"create table url(macaddr text NOT NULL UNIQUE, deviceurl text, proxyurl text);",
    //                         NULL,NULL,&errmsg);
	// if(ret != SQLITE_OK)
	// {
	// 	printf("%s\n",errmsg);
	// }	

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
	sin.sin_port = htons(SERVER_MEDIA_PORT);

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
			printf("Server Media:A new child connection.\n");
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
