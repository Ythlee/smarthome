//#include <string.h>
#include <QString>
#include "warning.h"
#include "ui_warning.h"
//#include "include/client.h"
Warning::Warning(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Warning)
{

    ui->setupUi(this);
    connect(ui->back, &QPushButton::pressed, this, &Warning::on_back_clicked);
}

Warning::~Warning()
{
    delete ui;
}
void Warning::on_back_clicked()
{
    this->close();
    emit send_back();
}
void Warning::deal_recv(QString mac, QString acc_name)
{
    qDebug() << "mac :" << mac;
    qDebug() << "name :" << acc_name;
    get_mac = mac;
    get_name = acc_name;
    ui->listWidget->clear();
    for (int i = 0; i < get_name.size(); i++) {
        name[i] = get_name[i].toLatin1();
    }
    for (int i = 0; i < get_mac.size(); i++) {
        focusMac[i] = get_mac[i].toLatin1();
    }

    memset(&msg, 0x0, sizeof(MsgStruct));
    DoDeviceNotice(alarmSockFd, &msg, name, focusMac);
    if (msg.custom1 == MSG_CUSTOM1_GET_ALARM_NOTICE_SUCCESS)
    {

        //qDebug() << "I am here 1";
        // printf("Get device notice success\n");
        int noticeNum = msg.custom2;
        printf("NoticeNum=%d\n",noticeNum);
        //qDebug() << noticeNum;
        for(int i=0; i< noticeNum; i++)
        {
            qDebug() << "I am here 2";
            char buffer[50] = {0};
            int noticeLen = 43;
            memcpy(buffer, msg.data+noticeLen*i, noticeLen);
            printf("NoticeData[%d]=%s\n", i, buffer);
            qDebug() << "Notice";
            QString s_buffer(buffer);
            ui->listWidget->addItem(QString("NoticeData[%1]=%2").arg(i).arg(s_buffer));
        }
        //qDebug() << "I am here 3";
    }
   // qDebug() << "I am here";
}
