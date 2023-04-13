#include "setdialog.h"
#include "ui_setdialog.h"
#include <QDebug>
SetDialog::SetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SetDialog)
{
    ui->setupUi(this);
}

SetDialog::~SetDialog()
{
    delete ui;
}

void SetDialog::on_ok_clicked()
{

    for (int i = 0; i < get_mac.size(); i++) {
        focusMac[i] = get_mac[i].toLatin1();
    }
    if (setwhich == "Mode")
    {
        if (ui->numedit->text() == "cold")
        {
            DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_COLD, NULL, focusMac);
            emit set_signal("mode", "cold");
            this->close();
        }
        else if(ui->numedit->text() == "hot")
        {
            DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_HOT, NULL, focusMac);
            emit set_signal("mode", "hot");
            this->close();
        }

    }
    else if (setwhich == "Temperature")
    {
        QString temp = ui->numedit->text();
        for (int i = 0; i < temp.size(); i++) {
            para[i] = temp[i].toLatin1();
        }
        DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_TEMP, para, focusMac);
        emit set_signal("temp", temp);
        this->close();
    }
    else if (setwhich == "Wind")
    {
        QString wind = ui->numedit->text();
        for (int i = 0; i < wind.size(); i++) {
            para[i] = wind[i].toLatin1();
        }
        DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_WIND, para, focusMac);
        emit set_signal("wind", wind);
        this->close();
    }
    else if (setwhich == "Fresh")
    {
        QString fresh = ui->numedit->text();
        for (int i = 0; i < fresh.size(); i++) {
            para[i] = fresh[i].toLatin1();
        }
        DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_REFRIGERATOR, MSG_CUSTOM2_SET_DEVICE_REF_FRESH, para, focusMac);
        emit set_frisignal("fridge_fresh", fresh);
        this->close();
    }
    else if (setwhich == "FridgeTemperature")
    {
        QString fridge_temp = ui->numedit->text();
        for (int i = 0; i < fridge_temp.size(); i++) {
            para[i] = fridge_temp[i].toLatin1();
        }

        DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_REFRIGERATOR, MSG_CUSTOM2_SET_DEVICE_REF_FREEZ, para, focusMac);
        emit set_frisignal("fridge_temp", fridge_temp);
        this->close();
    }
    else qDebug() << "This is Wrong";


   // num = ui->numedit->text().toInt();
   // this->close();
   // qDebug() << num;
    //emit num_singals(num);
    ui->numedit->clear();
}
void SetDialog::deal_sendsignal(QString str)
{
    //qDebug() << str;
    setwhich = str;
    if (setwhich == "Mode")
    {
        qDebug() << "This is SetMode";
        ui->settitle->setText("Please SetMode(hot/cold)");

    }
    else if (setwhich == "Temperature")
    {
        qDebug() << "This is SetTemperature";
        ui->settitle->setText("Please SetTemperature");
    }
    else if (setwhich == "Wind")
    {
        qDebug() << "This is SetWind";
        ui->settitle->setText("Please SetWind(low/middle/high)");
    }
    else if (setwhich == "Fresh")
    {
        qDebug() << "This is Fridge Fresh";
        ui->settitle->setText("Please SetFresh");
    }
    else if (setwhich == "FridgeTemperature")
    {
        qDebug() << "This is FridgeTemperature";
        ui->settitle->setText("Please SetFridgeTemperature");
    }
    else qDebug() << "This is Wrong";
}
void SetDialog::deal_mac(QString mac) {
    get_mac = mac;
}
