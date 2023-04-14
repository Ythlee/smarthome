#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "user_manager.h"
#include "user_register.h"
#include "user_login.h"
#include "../include/message.h"

int user_manager(int acceptfd, sqlite3 *db) {
    MsgStruct msg;
    memset(&msg, 0x0, sizeof(MsgStruct));

    while (recv(acceptfd, &msg, sizeof(msg), 0) > 0) {
        if (msg.protocol_type != MSG_PROTOCOL_C2S) {
            printf("protocol_type error:0x%x.\n", msg.protocol_type);
            continue;
        }
        switch (msg.data_type) {
            case MSG_DATA_ACCOUNT_REGISTER:
                user_register(acceptfd, &msg, db);
                break;
            case MSG_DATA_ACCOUNT_LOGIN:
                login_manager(acceptfd, &msg, db);
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
