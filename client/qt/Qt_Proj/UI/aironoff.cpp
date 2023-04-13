#include "aironoff.h"
#include "ui_aironoff.h"

AirOnOff::AirOnOff(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AirOnOff)
{
    ui->setupUi(this);
}

AirOnOff::~AirOnOff()
{
    delete ui;
}

void AirOnOff::on_on_clicked()
{
    for (int i = 0; i < get_mac.size(); i++) {
        focusMac[i] = get_mac[i].toLatin1();
    }
    DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_ON, NULL, focusMac);
    this->close();
    emit set_signal("onoff", "on");
}

void AirOnOff::on_off_clicked()
{
    for (int i = 0; i < get_mac.size(); i++) {
        focusMac[i] = get_mac[i].toLatin1();
    }
    DoSetDeviceStatus(deviceSockFd, &msg, MSG_CUSTOM1_SET_DEVICE_AIRCONDITION, MSG_CUSTOM2_SET_DEVICE_AIR_OFF, NULL, focusMac);
    this->close();
    emit set_signal("onoff", "off");
}
void AirOnOff::deal_mac(QString mac) {
    get_mac = mac;
}
