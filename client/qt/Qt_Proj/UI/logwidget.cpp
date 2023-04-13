#include "logwidget.h"
#include "ui_logwidget.h"
#include "include/message.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <QDialog>
#include <QDebug>
#include <QLabel>

LogWidget::LogWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogWidget)
{
    this->setStyleSheet("QPushButton{font:bold}");
    ui->setupUi(this);
    ui->key_edit->setEchoMode(QLineEdit::Password);

    connect(this, SIGNAL(singal(QString)), &firstwidget, SLOT(setAccount(QString)));
    connect(this, SIGNAL(res_signal(QString)), &logdialog, SLOT(deal_signal(QString)));
    connect(ui->login, &QPushButton::released,
           [=]()
            {
                QString input_name = ui->log_acc_edit->text();
                QString input_password = ui->key_edit->text();
                MsgStruct msg1;
                memset(&msg1, 0x0, sizeof(MsgStruct));
                int sockfd = userSockFd;
                int preSeq = GetCurSeq();
                char name[NAME_PASSWORD_LEN] = {0};
                char password[NAME_PASSWORD_LEN] = {0};
                for (int i = 0; i < input_name.size(); i++) {
                    name[i] = input_name[i].toLatin1();
                }
                for (int i = 0; i < input_password.size(); i++) {
                    password[i] = input_password[i].toLatin1();
                }
                MsgStruct *msg = &msg1;
                msg->version = MSG_VERSION;
                msg->header_len = MSG_HEADER_STABLE_LEN;
                msg->encrypt_type = MSG_ENCRYPT_NONE;
                msg->protocol_type = MSG_PROTOCOL_C2S;
                msg->data_type = MSG_DATA_ACCOUNT_LOGIN;
                msg->seq_num = preSeq;
                msg->frag_flag = MSG_FLAG_FRAG_NO;
                msg->frag_offset = 0;
                msg->custom1 = input_name.size();
                msg->custom2 = input_password.size();
                msg->total_len = MSG_HEADER_STABLE_LEN+msg->custom1+msg->custom2;
                msg->source_addr = 0x7F000001;
                msg->target_addr = 0x7F000001;
                msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
                memcpy(msg->data, name, msg->custom1);
                memcpy(msg->data + 1 + msg->custom1, password, msg->custom2);

                if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
                {
                    emit res_signal("Fail to send.");
                    logdialog.exec();
                    perror("Fail to send.\n");
                    return -1;
                }

                memset(msg, 0x0, sizeof(MsgStruct));
                if(recv(sockfd, msg, sizeof(MsgStruct), 0) < 0)
                {
                    emit res_signal("Fail to recv.");
                    logdialog.exec();
                    perror("Fail to recv.\n");
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
                if(msg->data_type != MSG_DATA_ACCOUNT_LOGIN)
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
                if(msg->custom1 == MSG_CUSTOM1_ACCOUNT_LOGIN_SUCCESS)
                {
                    emit res_signal("Login successfully!");
                    logdialog.exec();

                    //strcpy(userName, name);
                    emit singal(name);
                    this->close();
                    firstwidget.show();
                    printf("Login successfully!\n");
                    return 0;
                }
                else if(msg->custom1 == MSG_CUSTOM1_ACCOUNT_LOGIN_FAILURE)
                {
                    printf("Sorry！Login failed:\n");
                    switch (msg->custom2)
                    {
                        case MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_ACCOUNT:
                            {
                                emit res_signal("Login successfully!");
                                logdialog.exec();
                                printf("Wrong account or account not registered.\n");
                             } break;

                        case MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_PASSWORD:
                            {
                                emit res_signal("Wrong password.");
                                logdialog.exec();
                                printf("Wrong password.\n");
                             } break;
                        case MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_CONNECT:
                            printf("Abnormal connection.\n");
                            break;
                        case MSG_CUSTOM2_ACCOUNT_LOGIN_FAILURE_DATABASE:
                            printf("Database exception.\n");
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
               // memset(msg, 0x0, sizeof(MsgStruct));

                //登陆成果返回0，登陆失败返回-1
                return -1;



           }
           );
    connect(&firstwidget,  &FirstWidget::logout_singal, this, &LogWidget::deal_logout);
}

LogWidget::~LogWidget()
{
    delete ui;
}



void LogWidget::on_quit_clicked()
{
    this->close();
    emit log_close_singal();
}

void LogWidget::deal_logout()
{
    ui->key_edit->clear();
    ui->log_acc_edit->clear();
    this->show();
}

