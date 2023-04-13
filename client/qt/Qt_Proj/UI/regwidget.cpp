#include "regwidget.h"
#include "ui_regwidget.h"
#include "include/client.h"
#include <QDebug>

void DoRegister(int sockfd, MsgStruct *msg);

RegWidget::RegWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RegWidget)
{
    this->setStyleSheet("QPushButton{font:bold}");
    ui->setupUi(this);
    ui->key_edit->setEchoMode(QLineEdit::Password);
    connect(this, SIGNAL(res_signal(QString)), &regdialog, SLOT(deal_signal(QString)));
    connect(ui->reg, &QPushButton::released,
           [=]()
            {

                input_name = ui->acc_edit->text();
                input_password = ui->key_edit->text();
                MsgStruct msg1;
                memset(&msg1, 0x0, sizeof(MsgStruct));


                int preSeq = GetCurSeq();
                char name[NAME_PASSWORD_LEN] = {0};
                char password[NAME_PASSWORD_LEN] = {0};
                for (int i = 0; i < input_name.size(); i++) {
                    name[i] = input_name[i].toLatin1();
                }
                for (int i = 0; i < input_password.size(); i++) {
                    password[i] = input_password[i].toLatin1();
                }
                int sockfd = userSockFd;
                MsgStruct *msg = &msg1;
                msg->version = MSG_VERSION;
                msg->header_len = MSG_HEADER_STABLE_LEN;
                msg->encrypt_type = MSG_ENCRYPT_NONE;
                msg->protocol_type = MSG_PROTOCOL_C2S;
                msg->data_type = MSG_DATA_ACCOUNT_REGISTER;
                msg->seq_num = preSeq;
                msg->frag_flag = MSG_FLAG_FRAG_NO;
                msg->frag_offset = 0;
                msg->custom1 = input_name.size();
                msg->custom2 = input_password.size();
                msg->total_len = MSG_HEADER_STABLE_LEN+msg->custom1+msg->custom2;
                msg->source_addr = 0x7F000001;//GetIpMun(sourceIp)
                msg->target_addr = 0x7F000001;//GetIpMun(targetIp)
                msg->header_chk = GetChkSum((char*)(msg), MSG_HEADER_STABLE_LEN);
                memcpy(msg->data, name, msg->custom1);
                memcpy(msg->data + 1 + msg->custom1, password, msg->custom2);

                if(send(sockfd, msg, sizeof(MsgStruct), 0) < 0)
                {
                    emit res_signal("Fail to send.");
                    regdialog.exec();
                    perror("Fail to send.\n");
                    return;
                }

                memset(msg, 0x0, sizeof(MsgStruct));
                if(recv(sockfd, msg, sizeof(MsgStruct), 0) < 0)
                {
                    emit res_signal("Fail to recv.");
                    regdialog.exec();
                    perror("Fail to recv.\n");
                    return;
                }

                PrintMsg(msg);
                //Check Some Param
                if(msg->version != MSG_VERSION)
                {
                    printf("version error:0x%x.\n", msg->version);
                    return;
                }
                if(msg->encrypt_type != MSG_ENCRYPT_NONE)
                {
                    printf("encrypt_type error:0x%x.\n", msg->encrypt_type);
                    return;
                }
                if(msg->protocol_type != MSG_PROTOCOL_S2C)
                {
                    printf("protocol_type error:0x%x.\n", msg->protocol_type);
                    return;
                }
                if(msg->data_type != MSG_DATA_ACCOUNT_REGISTER)
                {
                    printf("data_type error:0x%x.\n", msg->data_type);
                    return;
                }
                if(msg->frag_flag != MSG_FLAG_FRAG_NO)
                {
                    printf("frag_flag error:0x%x.\n", msg->frag_flag);
                    return;
                }
                if(msg->seq_num != preSeq)
                {
                    printf("seq error:0x%x; preSeq:0x%x.\n", msg->seq_num, preSeq);
                    return;
                }
                if(msg->header_chk != GetChkSum((char*)(msg), msg->header_len))
                {
                    printf("header_chk error:0x%x.\n", msg->header_chk);
                    return;
                }

                //Get Result
                if(msg->custom1 == MSG_CUSTOM1_ACCOUNT_REG_SUCCESS)
                {
                    emit res_signal("Congratulations！You registered successfully!");
                    regdialog.exec();
                    printf("Congratulations！You registered successfully!\n");
                    this->close();
                    emit reg_close_singal();
                }
                else if(msg->custom1 == MSG_CUSTOM1_ACCOUNT_REG_FAILURE)
                {
                    emit res_signal("Sorry！You registered failed!");
                    regdialog.exec();
                    printf("Sorry！You registered failed:\n");
                    switch (msg->custom2)
                    {
                        case MSG_CUSTOM2_ACCOUNT_REG_FAILURE_ALREADY:
                            printf("Account already exist.\n");
                            break;
                        case MSG_CUSTOM2_ACCOUNT_REG_FAILURE_CONNECT:
                            printf("Connection failed.\n");
                            break;
                        case MSG_CUSTOM2_ACCOUNT_REG_FAILURE_DATABASE:
                            printf("Database error.\n");
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
           }
           );
}

RegWidget::~RegWidget()
{
    delete ui;
}

void RegWidget::on_quit_clicked()
{
    this->close();
    emit reg_close_singal();
}
