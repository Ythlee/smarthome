#!/bin/sh
while true
do
    count=`ps -ef | grep server_alarm | grep -v grep`
    if [ $? != "0" ]
    then
        echo "$(date "+%Y-%m-%d %H:%M:%S") > server_alarm restart" >> ./monitor_log.txt
        ../server_alarm/server_alarm &
    else 
        echo "$(date "+%Y-%m-%d %H:%M:%S") > server_alarm is runing" >> ./monitor_log.txt
    fi

    count=`ps -ef | grep server_user | grep -v grep`
    if [ $? != "0" ]
    then
        echo "$(date "+%Y-%m-%d %H:%M:%S") > server_user restart" >> ./monitor_log.txt
        ../server_user/server_user &
    else 
        echo "$(date "+%Y-%m-%d %H:%M:%S") > server_user is runing" >> ./monitor_log.txt
    fi

    count=`ps -ef | grep server_device | grep -v grep`
    if [ $? != "0" ]
    then
        echo "$(date "+%Y-%m-%d %H:%M:%S") > server_device restart" >> ./monitor_log.txt
        ../server_device/server_device &
    else 
        echo "$(date "+%Y-%m-%d %H:%M:%S") > server_device is runing" >> ./monitor_log.txt
    fi

    # count=`ps -ef | grep server_identify | grep -v grep`
    # if [ $? != "0" ]
    # then
    #     echo "$(date "+%Y-%m-%d %H:%M:%S") > server_identify restart" >> ./monitor_log.txt
    #     server_identify/server_identify &
    # else 
    #     echo "$(date "+%Y-%m-%d %H:%M:%S") > server_identify is runing" >> ./monitor_log.txt
    # fi

    count=`ps -ef | grep server_media | grep -v grep`
    if [ $? != "0" ]
    then
        echo "$(date "+%Y-%m-%d %H:%M:%S") > server_media restart" >> ./monitor_log.txt
        ../server_media/server_media &
    else 
        echo "$(date "+%Y-%m-%d %H:%M:%S") > server_media is runing" >> ./monitor_log.txt
    fi

    sleep 1
done