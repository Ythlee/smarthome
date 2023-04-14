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
#include "../include/message.h"

#define NAME_PASSWORD_LEN 128
#define DATABASE "../database/smarthome_server.db"

// #define DEBUG
static int GetChkSum(char *data, int dataLen) {
    int sum = 0;
    for (int i = 0; i < dataLen; i++) {
        sum += data[i];
    }
    return sum;
}

static void PrintMsg(MsgStruct *msg) {
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

static int CheckPattern(const char *values) {
    //检查name和password是否符合规范，必须是大小写字母、数字、下划线组成
    //正确返回0，错误返回-1
    char *p = NULL;
    unsigned char vlen = 0;
    unsigned char i = 0;

    p = (char *) values;
    vlen = strlen(values);
    for (i = 0; i < vlen; i++) {
        if (('0' < *p && *p < '9') ||
            ('a' < *p && *p < 'z') ||
            ('A' < *p && *p < 'Z') ||
            (*p == '_')) { ;
        } else {
            return 1;
        }
    }
    return 0;
}

void DoRegister(int acceptfd, MsgStruct *msg, sqlite3 *db) {
    printf("DoRegister\n");
    int curSeq = msg->seq_num;
    char *errmsg = NULL;
    char sql[256] = {0};
    char name[NAME_PASSWORD_LEN] = {0};
    char password[NAME_PASSWORD_LEN] = {0};

    PrintMsg(msg);

    //Check Some Param
    if (msg->version != MSG_VERSION) {
        printf("version error:0x%x.\n", msg->version);
        return;
    }
    if (msg->encrypt_type != MSG_ENCRYPT_NONE) {
        printf("encrypt_type error:0x%x.\n", msg->encrypt_type);
        return;
    }
    if (msg->protocol_type != MSG_PROTOCOL_C2S) {
        printf("protocol_type error:0x%x.\n", msg->protocol_type);
        return;
    }
    if (msg->frag_flag != MSG_FLAG_FRAG_NO) {
        printf("frag_flag error:0x%x.\n", msg->frag_flag);
        return;
    }
    if (msg->header_chk != GetChkSum((char *) (msg), msg->header_len)) {
        printf("header_chk error:0x%x.\n", msg->header_chk);
        return;
    }

    memcpy(name, msg->data, msg->custom1);
    memcpy(password, msg->data + 1 + msg->custom1, msg->custom2);

    //Todo
    //检查name和password是否符合规范，必须是大小写字母、数字、下划线组成
    if (CheckPattern(name) < 0) {
        printf("Name pattern:alphabet, number, underscore");
        return;
    }
    if (CheckPattern(password) < 0) {
        printf("Password pattern:alphabet, number, underscore");
        return;
    }

    //Get name and password
    sprintf(sql, "insert into user values('%s', '%s', 'no');", name, password);
    printf("%s\n", sql);

    memset(msg, 0x0, sizeof(MsgStruct));
    int ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if (ret != SQLITE_OK) {
        printf("%d,%s\n", ret, errmsg);
        msg->custom1 = MSG_CUSTOM1_ACCOUNT_REG_FAILURE;
        if (ret == SQLITE_CONSTRAINT) {
            //Name has already registered.
            msg->custom2 = MSG_CUSTOM2_ACCOUNT_REG_FAILURE_ALREADY;
        } else {
            //Other database errors
            msg->custom2 = MSG_CUSTOM2_ACCOUNT_REG_FAILURE_DATABASE;
        }
    } else {
        printf("client register ok!\n");
        msg->custom1 = MSG_CUSTOM1_ACCOUNT_REG_SUCCESS;
        msg->custom2 = 0xFF;
    }

    msg->version = MSG_VERSION;
    msg->header_len = MSG_HEADER_STABLE_LEN;
    msg->encrypt_type = MSG_ENCRYPT_NONE;
    msg->protocol_type = MSG_PROTOCOL_S2C;
    msg->data_type = MSG_DATA_ACCOUNT_REGISTER;
    msg->seq_num = curSeq;
    msg->frag_flag = MSG_FLAG_FRAG_NO;
    msg->frag_offset = 0;
    msg->total_len = MSG_HEADER_STABLE_LEN * 4;
    msg->source_addr = 0x7F000001;
    msg->target_addr = 0x7F000001;
    msg->header_chk = GetChkSum((char *) (msg), MSG_HEADER_STABLE_LEN);
    PrintMsg(msg);
    if (send(acceptfd, msg, sizeof(MsgStruct), 0) < 0) {
        perror("fail to send.\n");
        return;
    }
    printf("send ok!\n");
    return;
}

int DoLogin(int acceptfd, MsgStruct *msg, sqlite3 *db) {
    printf("DoLogin\n");
    int curSeq = msg->seq_num;
    char *errmsg = NULL;
    int nrow;
    int ncloumn;
    char **resultp;
    char sql[256] = {0};
    char name[NAME_PASSWORD_LEN] = {0};
    char password[NAME_PASSWORD_LEN] = {0};

    PrintMsg(msg);
    //Check Some Param
    if (msg->version != MSG_VERSION) {
        printf("version error:0x%x.\n", msg->version);
        return -1;
    }
    if (msg->encrypt_type != MSG_ENCRYPT_NONE) {
        printf("encrypt_type error:0x%x.\n", msg->encrypt_type);
        return -1;
    }
    if (msg->protocol_type != MSG_PROTOCOL_C2S) {
        printf("protocol_type error:0x%x.\n", msg->protocol_type);
        return -1;
    }
    if (msg->frag_flag != MSG_FLAG_FRAG_NO) {
        printf("frag_flag error:0x%x.\n", msg->frag_flag);
        return -1;
    }
    if (msg->header_chk != GetChkSum((char *) (msg), msg->header_len)) {
        printf("header_chk error:0x%x.\n", msg->header_chk);
        return -1;
    }
    memcpy(name, msg->data, msg->custom1);
    memcpy(password, msg->data + 1 + msg->custom1, msg->custom2);
    if (CheckPattern(name) < 0) {
        printf("Name pattern:alphabet, number, underscore");
        return -1;
    }
    if (CheckPattern(password) < 0) {
        printf("Password pattern:alphabet, number, underscore");
        return -1;
    }
    sprintf(sql, "select * from user where name = '%s' and password = '%s';", name, password);
    printf("%s\n", sql);
    memset(msg, 0x0, sizeof(MsgStruct));
    if (sqlite3_get_table(db, sql, &resultp, &nrow, &ncloumn, &errmsg) != SQLITE_OK) {
        printf("get_table fail!\n");
        printf("%s\n", errmsg);
        msg->custom1 = MSG_CUSTOM1_ACCOUNT_LOGIN_FAILURE;
        msg->custom2 = MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_DATABASE;
    } else {
        printf("get_table ok!\n");
        if (nrow == 1)// 查询成功，数据库中拥有此用户
        {
            strcpy(msg->data, "OK");
            msg->custom1 = MSG_CUSTOM1_ACCOUNT_REG_SUCCESS;
            msg->custom2 = 0xFF;
        } else if (nrow == 0) // 密码或者用户名错误
        {
            strcpy(msg->data, "usr/passwd wrong.");
            msg->custom1 = MSG_CUSTOM1_ACCOUNT_LOGIN_FAILURE;
            msg->custom2 = MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_PASSWORD;
        }
    }
    msg->version = MSG_VERSION;
    msg->header_len = MSG_HEADER_STABLE_LEN;
    msg->encrypt_type = MSG_ENCRYPT_NONE;
    msg->protocol_type = MSG_PROTOCOL_S2C;
    msg->data_type = MSG_DATA_ACCOUNT_LOGIN;
    msg->seq_num = curSeq;
    msg->frag_flag = MSG_FLAG_FRAG_NO;
    msg->frag_offset = 0;
    msg->total_len = MSG_HEADER_STABLE_LEN * 4;
    msg->source_addr = 0x7F000001;
    msg->target_addr = 0x7F000001;
    msg->header_chk = GetChkSum((char *) (msg), MSG_HEADER_STABLE_LEN);
    PrintMsg(msg);
    if (send(acceptfd, msg, sizeof(MsgStruct), 0) < 0) {
        perror("fail to send.\n");
        return -1;
    }
    printf("send ok!\n");
    return 0;
}

int DoClient(int acceptfd, sqlite3 *db) {
    MsgStruct msg;
    memset(&msg, 0x0, sizeof(MsgStruct));
    while (recv(acceptfd, &msg, sizeof(msg), 0) > 0) {
        if (msg.protocol_type != MSG_PROTOCOL_C2S) {
            printf("protocol_type error:0x%x.\n", msg.protocol_type);
            continue;
        }
        switch (msg.data_type) {
            //Register
            case MSG_DATA_ACCOUNT_REGISTER:
                DoRegister(acceptfd, &msg, db);
                break;
                //Login
            case MSG_DATA_ACCOUNT_LOGIN:
                //Todo
                DoLogin(acceptfd, &msg, db);
                break;
            default:
                printf("Invalid data msg.\n");
                break;
        }
        memset(&msg, 0x0, sizeof(MsgStruct));
    }

    printf("client exit.\n");
    close(acceptfd);
    exit(0);

    return 0;
}

void HandlerChildSig(int sig) {
    printf("Server_user HandlerChildSig IN.\n");
    if (sig == SIGCHLD) {
        waitpid(-1, NULL, WNOHANG);
    }
}

// ./server_user
int main(int argc, const char *argv[]) {
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
            DoClient(newfd, db);
        } else  // 父进程,用来接受客户端的请求的
        {
            close(newfd);
            signal(SIGCHLD, HandlerChildSig);
        }
    }

    return 0;
}