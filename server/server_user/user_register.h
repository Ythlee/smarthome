#ifndef __USER_REGISTER_H
#define __USER_REGISTER_H

#include "../include/message.h"


void user_register(int acceptfd, MsgStruct *msg, sqlite3 *db);
#endif