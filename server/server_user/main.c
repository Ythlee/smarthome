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
#include "user_info.h"
#include "user_manager.h"
#include "../include/message.h"


void HandlerChildSig(int sig)
{
    printf("Server_user HandlerChildSig IN.\n");
    if (sig == SIGCHLD) {
        waitpid(-1, NULL, WNOHANG);
    }
}

// ./server_user
int main(int argc, const char *argv[])
{
    int sockfd = -1;
    int newfd = -1;

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    sqlite3 *db;
    char *errmsg;
    pid_t pid;

    printf("Server User :IN\n");
    //打开数据库
    if (sqlite3_open(DATABASE, &db) != SQLITE_OK) {
        printf("%s\n", sqlite3_errmsg(db));
        return -1;
    }
    printf("open DATABASE success.\n");

    if (sqlite3_exec(db, "create table user(name text primary key, password text, loginstatus text);", NULL, NULL,
                     &errmsg) != SQLITE_OK) {
        printf("%s\n", errmsg);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("fail to socket.\n");
        return -1;
    }

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(SERVER_MASTER_IP);
    sin.sin_port = htons(SERVER_USER_PORT);

    if (bind(sockfd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        perror("fail to bind.\n");
        return -1;
    }

    // 将套接字设为监听模式
    if (listen(sockfd, 5) < 0) {
        perror("fail to listen.\n");
        return -1;
    }

    while (1) {
        if ((newfd = accept(sockfd, (struct sockaddr *) &sin, &len)) < 0) {
            perror("fail to accept");
            continue;
        }

        if ((pid = fork()) < 0) {
            perror("fail to fork");
            continue;
        } else if (pid == 0)  // 子进程
        {
            //处理客户端具体的消息
            printf("Server User:A new client connection.\n");
            close(sockfd);
            user_manager(newfd, db);
        } else  // 父进程,用来接受客户端的请求的
        {
            close(newfd);
            signal(SIGCHLD, HandlerChildSig);
        }
    }

    return 0;
}