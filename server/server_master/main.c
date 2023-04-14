/**
 * 主服务器
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <signal.h>

/**
 * @brief 打印帮助信息
 */
void print_help(void)
{
    printf("USAGE\n");
    printf("    -a    Start all servers.\n");
    printf("    -t    Enter test mode.\n");
    printf("\n");
}

void HandlerFunc(int sig)
{
    printf("OUCH! - I got signal %d\n", sig);
    system("./server_close.sh &");
    exit(0);
    // signal(SIGINT, SIG_DFL);
}

int main(int argc, char **argv)
{
    printf("Server Master %s:IN\n", __FUNCTION__);

    printf("Start all sub server\n");
    system("../server_user/server_user &");     // start user
    sleep(1);
    system("../server_device/server_device &"); // start device
    sleep(1);
    system("../server_alarm/server_alarm &");   // start alarm
    sleep(1);
    // system("server_identify/server_identify &"); // start identify
    system("../server_media/server_media &");   // start media
    system("./server_monitor.sh &");            //start detection

    signal(SIGINT, HandlerFunc);


    while(1) {
        sleep(5);
    }

    return 0;
}
