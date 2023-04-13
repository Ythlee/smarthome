#include "firstwidget.h"
#include "ui_firstwidget.h"
#include "include/client.h"
#include <QPushButton>
#include <QDebug>
FirstWidget::FirstWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FirstWidget)
{
    ui->setupUi(this);
    addDevice = new AddDevice;
    aircondition = new Aircondition;
    fridge = new Fridge;
    connect(ui->Add, &QPushButton::pressed, this, &FirstWidget::on_Add_clicked);
    connect(ui->Logout, &QPushButton::pressed, this, &FirstWidget::on_Logout_clicked);
    connect(ui->Choose, &QPushButton::pressed, this, &FirstWidget::Choose_clicked);
    //connect(ui->Add, &QPushButton::pressed, this, &FirstWidget::on_Add_clicked);
    //connect(addDevice, SIGNAL(add_mac(String)), this, SLOT(deal_mac(QString)));
    connect(addDevice, SIGNAL(device_name(QString)), this, SLOT(deal_devicename(QString)));
    connect(addDevice, SIGNAL(cancel_singal()), this, SLOT(deal_cancel()));
    connect(aircondition, SIGNAL(aircondition_return()), this, SLOT(deal_air_return()));
    connect(fridge, SIGNAL(fridge_return()), this, SLOT(deal_fridge_return()));
    //Q1:name_signal == send acc name
    connect(this, SIGNAL(name_signal(QString)), addDevice, SLOT(send_name(QString)));
    connect(this, SIGNAL(set_current_signals(QString, QString, QString, QString)), aircondition, SLOT(set_current(QString, QString, QString, QString)));
    connect(this,SIGNAL(send_mac(QString)),aircondition,SLOT(deal_mac(QString)));
    connect(this,SIGNAL(send_acc_name(QString)),aircondition,SLOT(deal_acc_name(QString)));
    connect(this,SIGNAL(send_acc_name(QString)),fridge,SLOT(deal_acc_name(QString)));

    connect(this, SIGNAL(set_current_signals(QString, QString)), fridge, SLOT(set_current(QString, QString)));
    connect(this,SIGNAL(send_mac(QString)),fridge,SLOT(deal_mac(QString)));
}



void QWidget::update()
{

}
FirstWidget::~FirstWidget()
{
    delete ui;
}

void FirstWidget::on_Quit_clicked()
{
    this->close();
}

void FirstWidget::on_Add_clicked()
{
    addDevice->show();
    emit name_signal(ui->Account_name->text());
    this->close();
}
void FirstWidget::Choose_clicked()
{
    QString mac = device[ui->listWidget->currentRow()];
    qDebug() << "choose air?" << mac[0] << mac[1];
    if (mac[0] == '0' && mac[1] == '1') {
        sleep(1);
        for (int i = 0; i < mac.size(); i++) {
            focusMac[i] = mac[i].toLatin1();
        }

        memset(&msg, 0x0, sizeof(MsgStruct));
        DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_AIRCONDITION, MSG_CUSTOM2_GET_DEVICE_AIR_ONOFF, focusMac);
        QString status_onoff(aircondition_status_onOff);
        qDebug() << "air_condition on/off: "<< status_onoff;

        memset(&msg, 0x0, sizeof(MsgStruct));
        DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_AIRCONDITION, MSG_CUSTOM2_GET_DEVICE_AIR_HOTCOLD, focusMac);
        QString status_hotcold(aircondition_status_hotCold);
        qDebug() << "air_condition status_hotcold: "<< status_hotcold;

        memset(&msg, 0x0, sizeof(MsgStruct));
        DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_AIRCONDITION, MSG_CUSTOM2_GET_DEVICE_AIR_TEMP, focusMac);
        QString status_temp(aircondition_status_temp);
        qDebug() << "air_condition status_temp: "<< status_temp;

        memset(&msg, 0x0, sizeof(MsgStruct));
        DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_AIRCONDITION, MSG_CUSTOM2_GET_DEVICE_AIR_WIND, focusMac);
        QString status_wind(aircondition_status_wind);
        qDebug() << "air_condition status_wind: "<< status_wind;

        aircondition->show();
        emit set_current_signals(status_onoff, status_hotcold, status_temp, status_wind);

    }
    if (mac[0] == '0' && mac[1] == '2') {

        sleep(1);
        for (int i = 0; i < mac.size(); i++) {
            focusMac[i] = mac[i].toLatin1();
        }

        memset(&msg, 0x0, sizeof(MsgStruct));
        DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_REFRIGERATOR, MSG_CUSTOM2_GET_DEVICE_REF_FRESH, focusMac);
        QString status_fresh(refrigerator_status_fresh);
        qDebug() << "fridge status_fresh: "<< status_fresh;

        memset(&msg, 0x0, sizeof(MsgStruct));
        DoGetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_GET_DEVICE_REFRIGERATOR, MSG_CUSTOM2_GET_DEVICE_REF_FREEZ, focusMac);
        QString status_temp(refrigerator_status_freez);
        qDebug() << "fridge status_temp: "<< status_temp;

        fridge->show();
        emit set_current_signals(status_fresh, status_temp);


    }
    emit send_acc_name(get_name);
    emit send_mac(mac);
    this->close();
}
void FirstWidget::on_Logout_clicked()
{
    this->close();
    emit logout_singal();
}

void FirstWidget::setAccount(QString str)
{
    ui->listWidget->clear();
    get_name = str;
    ui->Account_name->setText(str);
    int DeviceNum = 0;
    char getMac[1024] = {0};
    for (int i = 0; i < str.size(); i++) {
        name[i] = str[i].toLatin1();
    }
    memset(&msg, 0x0, sizeof(MsgStruct));
    DoQuery(deviceSockFd, &msg, name, &DeviceNum, getMac);//该函数中依次输出设备mac地址，并获取全部设备个数
    for (int i = 0; i < DeviceNum; i++) {
        char deviceMac[25] = {0};
        for (int j = 0; j < 23; j++) {
            deviceMac[j] = getMac[i * 23 + j];
        }
        printf("%d.%s\n", i+3, deviceMac);// printf("  2.%s   3.%s    *\n");
        QString mac(deviceMac);
        QString displaymac(deviceMac);
        for (int i = 0; i < device.size(); i++) {
            //if (mac != device[i])
            qDebug() << device[i];
        }
        if (mac[0] == '0' && mac[1] == '1') {
            displaymac = mac + "(Aircondition)";
        }
        else
        {
            displaymac = mac + "(Fridge)";
        }

        device.append(mac);
        qDebug() << "size:" << device.size();
        qDebug() << mac;
        ui->listWidget->addItem(displaymac);
    }

}

void FirstWidget::deal_cancel()
{
    this->show();
}

void FirstWidget::deal_devicename(QString str)
{
    ui->listWidget->setResizeMode(QListView::Adjust);   //自动适应布局
    //QListWidgetItem* it = new QListWidgetItem;
    //ui->listWidget->addItem(QString("%1").arg(str));
    qDebug() << "add mac:" << str;
    QString displaystr;
    if (str[0] == '0' && str[1] == '1') {
        displaystr = str + "(Aircondition)";
    }
    else
    {
        displaystr = str + "(Fridge)";
    }

    device.append(str);
    qDebug() << "size:" << device.size();
    qDebug() << str;
    ui->listWidget->addItem(displaystr);
    this->show();
}
void FirstWidget::deal_air_return()
{
    this->show();
}

void FirstWidget::deal_fridge_return()
{
    this->show();
}
