#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <pthread.h>
//#include "DynamicRTSPServer.hh"
//#include "version.hh"
#include "WW_H264VideoSource.h"
#include "WW_H264VideoServerMediaSubsession.h"

#include "../../include/message.h"

static char const* inputFileName = "test.264";

static int GetChkSum(char* data, int dataLen)
{
	int sum = 0;
	for(int i = 0; i < dataLen; i++)
	{
		sum += data[i];
	}
	return sum;
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,  
    char const* streamName, char const* inputFileName) 
{  
    char* url = rtspServer->rtspURL(sms);  
    UsageEnvironment& env = rtspServer->envir();  
    env << "\n\"" << streamName << "\" stream, from the file \""  
        << inputFileName << "\"\n";  
    env << "Play this stream using the URL \"" << url << "\"\n";  
    delete[] url;  
}

int _SubmitUrl(int fd, const char* sUrl)
{
	MsgStruct msg;
	msg.version = MSG_VERSION;
        msg.header_len = MSG_HEADER_STABLE_LEN;
        msg.encrypt_type = MSG_ENCRYPT_NONE;
        msg.protocol_type = MSG_PROTOCOL_D2S;
        msg.data_type = MSG_DATA_DEVICE_URL_NOTICE;
        msg.seq_num = 0;
        msg.frag_flag = MSG_FLAG_FRAG_NO;
        msg.frag_offset = 0;
        msg.custom1 = MSG_CUSTOM1_DEVICE_URL_NOTICE_START;
        msg.custom2 = 0xFF;
        msg.source_addr = 0x7F000001;
        msg.target_addr = 0x7F000001;
        msg.header_chk = GetChkSum((char*)(&msg), MSG_HEADER_STABLE_LEN);
        msg.total_len = MSG_HEADER_STABLE_LEN*4+strlen(sUrl);
        memcpy(msg.data, sUrl, strlen(sUrl));

	int nRet = send(fd, &msg, sizeof(MsgStruct), 0);
	if(nRet <= 0)
	{
		printf("send to server error \n");
	}
	return nRet > 0 ? 0 : -1;
}

static void* _ExecStartVideo(void *arg)
{
   int fd = *(int*)arg;
    /* 创建调度器 */
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

    UserAuthenticationDatabase* authDB = NULL;  

    /* 创建rtsp服务器 */
    RTSPServer* rtspServer = RTSPServer::createNew(*env, 8555, authDB);
    if (rtspServer == NULL)
    {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        return NULL;
    }

    //add live stream
    WW_H264VideoSource* videoSource = 0;

    //video name
    char streamName[] = "videoTest";
    char streamDescription[] = "streamend  by \"videoTest\" ";

    /* 创建会话 */
    ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName, streamDescription);
    
    /* 添加子会话 */
    sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(*env, inputFileName, True));
    //sms->addSubsession(WW_H264VideoServerMediaSubsession::createNew(*env, videoSource));
    
    /* 向服务器添加会话 */
    rtspServer->addServerMediaSession(sms);

    //答应信息到标准输出
    //announceStream(rtspServer, sms, streamName, inputFileName);

    //试图为RTSP-over-HTTP通道创建一个HTTP服务器.
    if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080))
    {
        *env << "\n(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling.)\n";
    }
    else
    {
        *env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
    }

    char* url = rtspServer->rtspURL(sms);
    *env << "Play this stream using the URL \"" << url << "\"\n";

    _SubmitUrl(fd, url);
    
    delete[] url;

    /* 循环处理事件 */
    env->taskScheduler().doEventLoop();

    rtspServer->removeServerMediaSession(sms);

    Medium::close(rtspServer);

    env->reclaim();

    delete scheduler;

    return NULL;
}

/* link video server*/
int ConnectVideoServer(const char* sIP, const int nPort, int &nFd)
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

	nFd = sock;

	return 0;
}

int StartVideo(int* fd)
{
	pthread_t thread_t;
	pthread_create(&thread_t, NULL, _ExecStartVideo, fd);
	//_ExecStartVideo(fd);
	return 0;
}

int StopVideo(int fd)
{
	close(fd);
	return 0;
}
