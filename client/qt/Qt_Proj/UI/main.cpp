#include "mainwidget.h"
#include "logwidget.h"
#include "regwidget.h"
#include "firstwidget.h"
#include <QApplication>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/client.h"


#define NAME_PASSWORD_LEN 128

int client_init();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWidget w;

    client_init();
    w.show();

    return a.exec();
}

int client_init()
{
        memset(&msg, 0x0, sizeof(MsgStruct));
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

}
