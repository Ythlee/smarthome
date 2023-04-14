#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "user_register.h"
#include "user_login.h"
#include "user_info.h"
#include "../include/message.h"


void user_register(int acceptfd, MsgStruct *msg, sqlite3 *db) {
    printf("DoRegister\n");
    int curSeq = msg->seq_num;
    char *errmsg = NULL;
    char sql[512] = {0};
    char name[NAME_PASSWORD_LEN] = {0};
    char password[NAME_PASSWORD_LEN] = {0};

    user_info(msg);

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
    if (msg->header_chk != get_chksum((char *) (msg), msg->header_len)) {
        printf("header_chk error:0x%x.\n", msg->header_chk);
        return;
    }

    memcpy(name, msg->data, msg->custom1);
    memcpy(password, msg->data + 1 + msg->custom1, msg->custom2);

    //检查name和password是否符合规范，必须是大小写字母、数字、下划线组成
    if (check_legality(name) < 0) {
        printf("Name pattern:alphabet, number, underscore");
        return;
    }
    if (check_legality(password) < 0) {
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
    msg->header_chk = get_chksum((char *) (msg), MSG_HEADER_STABLE_LEN);
    user_info(msg);
    if (send(acceptfd, msg, sizeof(MsgStruct), 0) < 0) {
        perror("fail to send.\n");
        return;
    }
    printf("send ok!\n");
    return;
}
