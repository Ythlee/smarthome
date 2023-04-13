#include "adddevice.h"
#include "ui_adddevice.h"
#include <QDebug>
AddDevice::AddDevice(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddDevice)
{
    this->setWindowTitle("Please Add Your Device");
    ui->setupUi(this);

    connect(ui->cancel, &QPushButton::pressed, this, &AddDevice::on_cancel_clicked);
    connect(ui->add, &QPushButton::released,
            [=]()
            {
                    memset(focusMac, 0x0, 32);
                    memset(&msg, 0x0, sizeof(MsgStruct));
                    qDebug() << str;
                    for (int i = 0; i < str.size(); i++) {
                        name[i] = str[i].toLatin1();
                    }
                    int sockfd = deviceSockFd;

                    /*DoAddDevice(deviceSockFd, &msg, name, focusMac);
                    printf("Add mac:	%s.\n", focusMac);
                    */
                    MsgStruct msg1;
                    memset(&msg1, 0x0, sizeof(MsgStruct));
                    MsgStruct *msg = &msg1;
                    int preSeq = GetCurSeq();
                    msg->version = MSG_VERSION;
                    msg->header_len = MSG_HEADER_STABLE_LEN;
                    msg->encrypt_type = MSG_ENCRYPT_NONE;
                    msg->protocol_type = MSG_PROTOCOL_C2S;
                    msg->data_type = MSG_DATA_ADD_DEVICE;
                    msg->seq_num = preSeq;
                    msg->frag_flag = MSG_FLAG_FRAG_NO;
                    msg->frag_offset = 0;
                    msg->custom1 = str.size();
                    msg->custom2 = 0;
                    msg->total_len = MSG_HEADER_STABLE_LEN+msg->custom1;
                    msg->source_addr = 0x7F000001;//GetIpMun(sourceIp)
                    msg->target_addr = 0x7F000001;//GetIpMun(targetIp)
                    msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
                    memcpy(msg->data, name, msg->custom1);

                    if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
                    {
                        perror("fail to send 1234.\n");
                        return -1;
                    }

                    memset(msg, 0x0, sizeof(MsgStruct));
                    if(recv(sockfd, msg, sizeof(MsgStruct), 0) < 0)
                    {
                        perror("fail to recv.\n");
                        return -1;
                    }

                    PrintMsg(msg);
                    //Check Some Param
                    if(msg->version != MSG_VERSION)
                    {
                        printf("version error:0x%x.\n", msg->version);
                        return -1;
                    }
                    if(msg->encrypt_type != MSG_ENCRYPT_NONE)
                    {
                        printf("encrypt_type error:0x%x.\n", msg->encrypt_type);
                        return -1;
                    }
                    if(msg->protocol_type != MSG_PROTOCOL_S2C)
                    {
                        printf("protocol_type error:0x%x.\n", msg->protocol_type);
                        return -1;
                    }
                    if(msg->data_type != MSG_DATA_ADD_DEVICE)
                    {
                        printf("data_type error:0x%x.\n", msg->data_type);
                        return -1;
                    }
                    if(msg->frag_flag != MSG_FLAG_FRAG_NO)
                    {
                        printf("frag_flag error:0x%x.\n", msg->frag_flag);
                        return -1;
                    }
                    if(msg->seq_num != preSeq)
                    {
                        printf("seq error:0x%x; preSeq:0x%x.\n", msg->seq_num, preSeq);
                        return -1;
                    }
                    if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
                    {
                        printf("header_chk error:0x%x.\n", msg->header_chk);
                        return -1;
                    }

                    //Get Result
                    if(msg->custom1 == MSG_CUSTOM1_ADD_DEVICE_SUCCESS)
                    {
                        printf("New device added successfully!\n");
                        strcpy(focusMac, msg->data);

                        QString str_mac(focusMac);
                        qDebug() << str_mac;
                        emit device_name(str_mac);
                        this->close();
                        return 0;
                    }
                    else if(msg->custom1 == MSG_CUSTOM1_ADD_DEVICE_FAILURE)
                    {
                        printf("Sorry！Failed to add device:\n");
                        switch (msg->custom2)
                        {
                            case MSG_CUSTOM2_ADD_DEVICE_FAILURE_MACADDR:
                                printf("Wrong mac address.\n");
                                break;
                            case MSG_CUSTOM2_ADD_DEVICE_FAILURE_ALREADY:
                                printf("Device has been added.\n");
                                break;
                            case MSG_CUSTOM2_ADD_DEVICE_FAILURE_CONNECT:
                                printf("Abnormal connection.\n");
                                break;
                            case MSG_CUSTOM2_ADD_DEVICE_FAILURE_DATABASE:
                                printf("Database exception.\n");
                                break;
                            case MSG_CUSTOM2_ADD_DEVICE_FAILURE_DEVICE:
                                printf("Abnormal device.\n");
                                break;
                            default:
                                printf("unknow reason:0x%x\n", msg->custom2);
                                break;
                        }
                    }
                    else
                    {
                        printf("Return unknow status:0x%x\n",msg->custom1);
                    }
                    memset(msg, 0x0, sizeof(MsgStruct));
            }
            );


}

AddDevice::~AddDevice()
{
    delete ui;
}

void AddDevice::on_cancel_clicked()
{
    this->close();
    emit cancel_singal();
}

void AddDevice::send_name(QString name)
{
    str = name;
}
