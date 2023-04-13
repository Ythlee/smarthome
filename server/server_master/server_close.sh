#!/bin/bash
count=`ps -ef | grep server_monitor | grep -v grep`
if [ $? == "0" ]
then
    exec=`ps -ef | grep server_monitor | grep -v grep | awk '{print $2}' | xargs kill`
fi

count=`ps -ef | grep server_alarm | grep -v grep`
if [ $? == "0" ]
then
    exec=`ps -ef | grep server_alarm | grep -v grep | awk '{print $2}' | xargs kill`
fi

count=`ps -ef | grep server_user | grep -v grep`
if [ $? == "0" ]
then
    exec=`ps -ef | grep server_user | grep -v grep | awk '{print $2}' | xargs kill`
fi

count=`ps -ef | grep server_device | grep -v grep`
if [ $? == "0" ]
then
    exec=`ps -ef | grep server_device | grep -v grep | awk '{print $2}' | xargs kill`
fi

# count=`ps -ef | grep server_identify | grep -v grep`
# if [ $? == "0" ]
# then
#     exec=`ps -ef | grep server_identify | grep -v grep | awk '{print $2}' | xargs kill`
# fi

count=`ps -ef | grep server_media | grep -v grep`
if [ $? == "0" ]
then
    exec=`ps -ef | grep server_media | grep -v grep | awk '{print $2}' | xargs kill`
fi