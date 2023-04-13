#include "media.h"
#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "../include/message.h"


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


