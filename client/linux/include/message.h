#ifndef INCLUDE_MESSAGE_H
#define INCLUDE_MESSAGE_H
/*==================================Server Port Number========================================*/
#define SERVER_MASTER_IP     "127.0.0.1"
#define SERVER_USER_PORT     20000
#define SERVER_DEVICE_PORT   20001
#define SERVER_ALARM_PORT    20002
#define SERVER_IDENTIFY_PORT 20003
#define SERVER_MEDIA_PORT    20004
/*==================================Message Data Structure====================================*/
// Version
#define MSG_VERSION 0x01

//Encryption type
#define MSG_ENCRYPT_NONE 0x0
#define MSG_ENCRYPT_RC 0x1

//Protocol type
//Client to Server
#define MSG_PROTOCOL_C2S 0x0
//Server to Client
#define MSG_PROTOCOL_S2C 0x1
//Server to Device
#define MSG_PROTOCOL_S2D 0x2
//Device to Server
#define MSG_PROTOCOL_D2S 0x3

//Date type
//Account Register
#define MSG_DATA_ACCOUNT_REGISTER 0x0
//Account Login
#define MSG_DATA_ACCOUNT_LOGIN 0x1
//Query Device
#define MSG_DATA_QUERY_DEVICE 0x2
//Add Device
#define MSG_DATA_ADD_DEVICE 0x3
//Set Device Status
#define MSG_DATA_SET_DEVICE_STATUS 0x4
//Get Device Status
#define MSG_DATA_GET_DEVICE_STATUS 0x5
//Device Alarm Notify
#define MSG_DATA_DEVICE_ALARM_NOTIFY 0x6
//Name Device
#define MSG_DATA_NAME_DEVICE 0x7
//Device Connection
#define MSG_DATA_DEVICE_CONNECTION 0x8
//Get Alarm Notice
#define MSG_DATA_GET_ALARM_NOTICE 0x9
//Device Url Notice
#define MSG_DATA_DEVICE_URL_NOTICE 0xA
//Get Proxy Url
#define MSG_DATA_GET_PROXY_URL 0xB

//Message Flag
//Message don't fragment
#define MSG_FLAG_FRAG_NO 0x0
//Message fragment
#define MSG_FLAG_FRAG_YES 0x1

//Custom1,Custom2
//Account Register
#define MSG_CUSTOM1_ACCOUNT_REG_SUCCESS 0x0
#define MSG_CUSTOM1_ACCOUNT_REG_FAILURE 0x1
#define MSG_CUSTOM2_ACCOUNT_REG_FAILURE_ALREADY  0x0
#define MSG_CUSTOM2_ACCOUNT_REG_FAILURE_CONNECT  0x1
#define MSG_CUSTOM2_ACCOUNT_REG_FAILURE_DATABASE 0x2
//Account Login
#define MSG_CUSTOM1_ACCOUNT_LOGIN_SUCCESS 0x0
#define MSG_CUSTOM1_ACCOUNT_LOGIN_FAILURE 0x1
#define MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_ACCOUNT  0x0
#define MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_PASSWORD 0x1
#define MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_CONNECT  0x2
#define MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_DATABASE 0x3
//Query Device
#define MSG_CUSTOM1_QUERY_DEVICE_SUCCESS 0x0
#define MSG_CUSTOM1_QUERY_DEVICE_FAILURE 0x1
#define MSG_CUSTOM2_QUERY_DEVICE_FAILURE_ACCOUNT  0x0
#define MSG_CUSTOM2_QUERY_DEVICE_FAILURE_CONNECT  0x1
#define MSG_CUSTOM2_QUERY_DEVICE_FAILURE_DATABASE 0x2
//Add Device
#define MSG_CUSTOM1_ADD_DEVICE_SUCCESS 0x0
#define MSG_CUSTOM1_ADD_DEVICE_FAILURE 0x1
#define MSG_CUSTOM2_ADD_DEVICE_FAILURE_MACADDR  0x0
#define MSG_CUSTOM2_ADD_DEVICE_FAILURE_ALREADY  0x1
#define MSG_CUSTOM2_ADD_DEVICE_FAILURE_CONNECT  0x2
#define MSG_CUSTOM2_ADD_DEVICE_FAILURE_DATABASE 0x3
#define MSG_CUSTOM2_ADD_DEVICE_FAILURE_DEVICE   0x4
//Set Device Status
#define MSG_CUSTOM1_SET_DEVICE_AIRCONDITION 0x0
#define MSG_CUSTOM1_SET_DEVICE_REFRIGERATOR 0x1
#define MSG_CUSTOM2_SET_DEVICE_AIR_ON    0x0
#define MSG_CUSTOM2_SET_DEVICE_AIR_OFF   0x1
#define MSG_CUSTOM2_SET_DEVICE_AIR_HOT   0x2
#define MSG_CUSTOM2_SET_DEVICE_AIR_COLD  0x3
#define MSG_CUSTOM2_SET_DEVICE_AIR_TEMP  0x4
#define MSG_CUSTOM2_SET_DEVICE_AIR_WIND  0x5
#define MSG_CUSTOM2_SET_DEVICE_REF_FRESH 0x0
#define MSG_CUSTOM2_SET_DEVICE_REF_FREEZ 0x1
#define MSG_CUSTOM1_SET_DEVICE_STATUS_SUCCESS 0x0
#define MSG_CUSTOM1_SET_DEVICE_STATUS_FAILURE 0x1
#define MSG_CUSTOM2_SET_DEVICE_STATUS_FAILURE_MACADDR 0x0
#define MSG_CUSTOM2_SET_DEVICE_STATUS_FAILURE_COMMAND 0x1
#define MSG_CUSTOM2_SET_DEVICE_STATUS_FAILURE_CONNECT 0x2
#define MSG_CUSTOM2_SET_DEVICE_STATUS_FAILURE_DEVICE  0x3
//Get Device Status
#define MSG_CUSTOM1_GET_DEVICE_AIRCONDITION 0x0
#define MSG_CUSTOM1_GET_DEVICE_REFRIGERATOR 0x1
#define MSG_CUSTOM2_GET_DEVICE_AIR_ONOFF    0x0
#define MSG_CUSTOM2_GET_DEVICE_AIR_HOTCOLD  0x1
#define MSG_CUSTOM2_GET_DEVICE_AIR_TEMP     0x2
#define MSG_CUSTOM2_GET_DEVICE_AIR_WIND     0x3
#define MSG_CUSTOM2_GET_DEVICE_REF_FRESH    0x0
#define MSG_CUSTOM2_GET_DEVICE_REF_FREEZ    0x1
#define MSG_CUSTOM1_GET_DEVICE_STATUS_SUCCESS 0x0
#define MSG_CUSTOM1_GET_DEVICE_STATUS_FAILURE 0x1
#define MSG_CUSTOM2_GET_DEVICE_STATUS_FAILURE_MACADDR 0x0
#define MSG_CUSTOM2_GET_DEVICE_STATUS_FAILURE_COMMAND 0x1
#define MSG_CUSTOM2_GET_DEVICE_STATUS_FAILURE_CONNECT 0x2
#define MSG_CUSTOM2_GET_DEVICE_STATUS_FAILURE_DEVICE  0x3
//Device Alarm Notify
#define MSG_CUSTOM1_DEVICE_ALARM_NOTIFY_AIRCONDITION 0x0
#define MSG_CUSTOM1_DEVICE_ALARM_NOTIFY_REFRIGERATOR 0x1
#define MSG_CUSTOM2_DEVICE_ALARM_NOTIFY_AIR_CLOSE 0x0
#define MSG_CUSTOM2_DEVICE_ALARM_NOTIFY_AIR_TEMP  0x1
#define MSG_CUSTOM2_DEVICE_ALARM_NOTIFY_REF_FOOD  0x0
#define MSG_CUSTOM2_DEVICE_ALARM_NOTIFY_REF_FRESH 0x1
#define MSG_CUSTOM2_DEVICE_ALARM_NOTIFY_REF_FREEZ 0x2
//Name Device
#define MSG_CUSTOM1_NAME_DEVICE_SUCCESS 0x0
#define MSG_CUSTOM1_NAME_DEVICE_FAILURE 0x1
#define MSG_CUSTOM2_NAME_DEVICE_FAILURE_FORMAT   0x0
#define MSG_CUSTOM2_NAME_DEVICE_FAILURE_CONNECT  0x1
#define MSG_CUSTOM2_NAME_DEVICE_FAILURE_DATABASE 0x2
//Device Connection
#define MSG_CUSTOM1_DEVICE_CONNECTION_SUCCESS 0x0
#define MSG_CUSTOM1_DEVICE_CONNECTION_FAILURE 0x1
#define MSG_CUSTOM2_DEVICE_CONNECTION_FAILURE_MAC    0x0
#define MSG_CUSTOM2_DEVICE_CONNECTION_FAILURE_NOTADD 0x1
#define MSG_CUSTOM2_DEVICE_CONNECTION_FAILURE_NET    0x2
//Get Alarm Notice
#define MSG_CUSTOM1_GET_ALARM_NOTICE_SUCCESS 0x0
#define MSG_CUSTOM1_GET_ALARM_NOTICE_FAILURE 0x1
#define MSG_CUSTOM2_GET_ALARM_NOTICE_ACCOUNT  0x0
#define MSG_CUSTOM2_GET_ALARM_NOTICE_CONNECT  0x1
#define MSG_CUSTOM2_GET_ALARM_NOTICE_DATABASE 0x2
//Device Url Notice
#define MSG_CUSTOM1_DEVICE_URL_NOTICE_START 0x0
#define MSG_CUSTOM1_DEVICE_URL_NOTICE_STOP  0x1
#define MSG_CUSTOM1_DEVICE_URL_NOTICE_SUCCESS 0x0
#define MSG_CUSTOM1_DEVICE_URL_NOTICE_FAILURE 0x1
#define MSG_CUSTOM2_DEVICE_URL_NOTICE_FAILURE_MAC      0x0
#define MSG_CUSTOM2_DEVICE_URL_NOTICE_FAILURE_URL      0x1
#define MSG_CUSTOM2_DEVICE_URL_NOTICE_FAILURE_NOTADD   0x2
#define MSG_CUSTOM2_DEVICE_URL_NOTICE_FAILURE_NET      0x3
#define MSG_CUSTOM2_DEVICE_URL_NOTICE_FAILURE_DATABASE 0x4
//Get Proxy Url
#define MSG_CUSTOM1_GET_PROXY_URL_SUCCESS 0x0
#define MSG_CUSTOM1_GET_PROXY_URL_FAILURE 0x1
#define MSG_CUSTOM2_GET_PROXY_URL_ACCOUNT  0x0
#define MSG_CUSTOM2_GET_PROXY_URL_CONNECT  0x1
#define MSG_CUSTOM2_GET_PROXY_URL_DATABASE 0x2

//The message struct of communication
#define MSG_HEADER_STABLE_LEN 5
#define MSG_DATA_MAX_LEN 512
typedef struct {
	int version:4;
    int header_len:4;
    int encrypt_type:4;
    int protocol_type:4;
    int total_len:16;
    int data_type:8;
    int seq_num:8;
    int frag_flag:3;
    int frag_offset:13;
    int custom1:8;
    int custom2:8;
    int header_chk:16;
    int source_addr;
    int target_addr;
	char data[MSG_DATA_MAX_LEN];
}MsgStruct;
/*==========================================================================================*/

#endif