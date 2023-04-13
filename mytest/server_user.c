#include <stdio.h>
#include <sqlite3.h>


int main(int argc, char *argv[])
{
    char *errmsg;
    sqlite3 *db;

    if(sqlite3_open("./smarthomedb.db", &db) != SQLITE_OK) {
        printf("%s\n", sqlite3_errmsg(db));
        return -1;
    }

    printf("open db succeed\n");

    if(sqlite3_exec(db, "create table user(name text primary key, password text, loginstatus text);", NULL, NULL, &errmsg)) {
        printf("%s\n", errmsg);
        return -1;
    }

    return 0;
}
