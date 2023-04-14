#ifndef __USER_MANAGER_H
#define __USER_MANAGER_H
#include <sqlite3.h>

int user_manager(int accepfd, sqlite3 *db);

#endif
