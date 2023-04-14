#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sqlite3.h>
#include "user_login.h"
#include "user_info.h"


int get_chksum(char *data, int data_len)
{
    int i = 0;
    int chksum = 0;
    for (i = 0; i < data_len; i++) {
        chksum += data[i];
    }
    return chksum;
}


/**
 * @brief  检查用户名和密码是否合法
 * @param values
 * @return 0 合法 1 不合法
 */
int check_legality(const char *values)
{
    int i = 0;
    while (values[i] != '\0') {
        if ((values[i] >= 'a' && values[i] <= 'z') ||
            (values[i] >= 'A' && values[i] <= 'Z') ||
            (values[i] >= '0' && values[i] <= '9') ||
            (values[i] == '_')) {
            i++;
        } else {
            return 1;
        }
    }
    return 0;
}

int login_manager(int acceptfd, MsgStruct *msg, sqlite3 *db) {
    printf("DoLogin\n");
    int curSeq = msg->seq_num;
    char *errmsg = NULL;
    int nrow;
    int ncloumn;
    char **resultp;
    char sql[512] = {0};
    char name[NAME_PASSWORD_LEN] = {0};
    char password[NAME_PASSWORD_LEN] = {0};

    user_info(msg);
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
    if (msg->header_chk != get_chksum((char *) (msg), msg->header_len)) {
        printf("header_chk error:0x%x.\n", msg->header_chk);
        return -1;
    }
    memcpy(name, msg->data, msg->custom1);
    memcpy(password, msg->data + 1 + msg->custom1, msg->custom2);
    if (check_legality(name) < 0) {
        printf("Name pattern:alphabet, number, underscore");
        return -1;
    }
    if (check_legality(password) < 0) {
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
    msg->header_chk = get_chksum((char *) (msg), MSG_HEADER_STABLE_LEN);
    user_info(msg);
    if (send(acceptfd, msg, sizeof(MsgStruct), 0) < 0) {
        perror("fail to send.\n");
        return -1;
    }
    printf("send ok!\n");
    return 0;
}
