#ifndef __USER_LOGIN_H
#define __USER_LOGIN_H

#include <sqlite3.h>

#include "../include/message.h"

int get_chksum(char *data, int data_len);
int check_legality(const char *values);
int login_manager(int acceptfd, MsgStruct *msg, sqlite3 *db);


#endif
