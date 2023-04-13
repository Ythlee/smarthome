#include "aircondition.h"
#include "ui_aircondition.h"
#include <QDialog>
#include <QPushButton>
#include <QDebug>
Aircondition::Aircondition(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Aircondition)
{
    ui->setupUi(this);
    aironoff = new AirOnOff(this);
    setdialog = new SetDialog(this);
    warning = new Warning;
    connect(ui->back, &QPushButton::pressed, this, &Aircondition::on_return_clicked);
    connect(ui->quit, &QPushButton::pressed, this, &Aircondition::on_quit_clicked);
    connect(ui->on_off, &QPushButton::pressed, this, &Aircondition::on_on_off_clicked);
    connect(ui->mode, &QPushButton::pressed, this, &Aircondition::on_mode_clicked);
    connect(ui->temperature, &QPushButton::pressed, this, &Aircondition::on_temp_clicked);
    connect(ui->wind, &QPushButton::pressed, this, &Aircondition::on_wind_clicked);
    connect(ui->warn, &QPushButton::pressed, this, &Aircondition::on_warning_clicked);

    connect(warning,SIGNAL(send_back()), this, SLOT(deal_back()));
    connect(this, SIGNAL(sendsignal(QString)), setdialog, SLOT(deal_sendsignal(QString)));
    connect(this, SIGNAL(send_mac(QString)), setdialog, SLOT(deal_mac(QString)));
    connect(this, SIGNAL(send_mac(QString)), aironoff, SLOT(deal_mac(QString)));
    connect(this, SIGNAL(send_to_warning(QString, QString)), warning, SLOT(deal_recv(QString, QString)));
    connect(aironoff, SIGNAL(set_signal(QString, QString)), this, SLOT(deal_set_signal(QString, QString)));
    connect(setdialog, SIGNAL(set_signal(QString, QString)), this, SLOT(deal_set_signal(QString, QString)));

}

Aircondition::~Aircondition()
{
    delete ui;
}

void Aircondition::deal_back()
{
    this->show();
}


void Aircondition::on_mode_clicked() {
    emit sendsignal("Mode");
    setdialog->exec();

}
void Aircondition::on_temp_clicked() {
    emit sendsignal("Temperature");
    setdialog->exec();

}
void Aircondition::on_wind_clicked() {
    emit sendsignal("Wind");
    setdialog->exec();

}
void Aircondition::on_warning_clicked()
{
    emit send_to_warning(get_mac, get_name);
    //warning = new Warning(this);
    warning->show();
    this->hide();
}

void Aircondition::on_return_clicked() {
    this->close();
    emit aircondition_return();
}
void Aircondition::on_quit_clicked() {
    this->close();
}

void Aircondition::on_on_off_clicked() {
    aironoff->exec();
    emit send_mac(get_mac);
}

void Aircondition::set_current(QString status_onoff,QString status_hotcold,QString status_temp,QString status_wind)
{
    ui->current_onoff->setText(status_onoff);
    ui->current_mode->setText(status_hotcold);
    ui->current_temp->setText(status_temp);
    ui->current_wind->setText(status_wind);
}

void Aircondition::deal_mac(QString mac)
{
    get_mac = mac;
}

void Aircondition::deal_acc_name(QString name)
{
    get_name = name;
}

void Aircondition::deal_set_signal(QString set, QString set_current)
{
    if (set == "onoff")
    {
        ui->current_onoff->setText(set_current);
    }
    if (set == "mode") {
        //qDebug() << "setMode";
        ui->current_mode->setText(set_current);
    }
    if (set == "temp") {
        ui->current_temp->setText(set_current);
    }
    if (set == "wind") {
        ui->current_wind->setText(set_current);
    }
}
