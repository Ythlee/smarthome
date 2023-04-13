#include "fridge.h"
#include "ui_fridge.h"
#include "include/client.h"
#include "include/media.h"
#include <QProcess>
Fridge::Fridge(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Fridge)
{
    setdialog = new SetDialog(this);
    ui->setupUi(this);
    fridge_warning = new FridgeWarning;
    connect(ui->back, &QPushButton::pressed, this, &Fridge::on_return_clicked);
    connect(ui->quit, &QPushButton::pressed, this, &Fridge::on_quit_clicked);
    connect(this, SIGNAL(sendsignal(QString)), setdialog, SLOT(deal_sendsignal(QString)));
    connect(this, SIGNAL(send_mac(QString)), setdialog, SLOT(deal_mac(QString)));
    connect(ui->warn, &QPushButton::pressed, this, &Fridge::on_warning_clicked);
    connect(ui->video, &QPushButton::released, this, &Fridge::video_clicked);
    connect(this, SIGNAL(send_to_warning(QString, QString)), fridge_warning, SLOT(deal_recv(QString, QString)));
    connect(fridge_warning,SIGNAL(send_back()), this, SLOT(deal_back()));
    connect(setdialog, SIGNAL(set_frisignal(QString, QString)), this, SLOT(deal_set_signal(QString, QString)));
}


Fridge::~Fridge()
{
    delete ui;
}
void Fridge::on_return_clicked() {
    this->close();
    emit fridge_return();
}
void Fridge::on_quit_clicked() {
    this->close();
}

void Fridge::on_fresh_clicked() {
    emit sendsignal("Fresh");
    setdialog->exec();

}
void Fridge::on_temp_clicked() {
    emit sendsignal("FridgeTemperature");
    setdialog->exec();

}
void Fridge::video_clicked() {

    RequestVideoUrl(mediaSockFd, url);
    char exec[128] = {0};
    sprintf(exec, "vlc %s", url);
    printf("start vlc play : %s \n", exec);
   // system("/usr/bin/vlc");
    system(exec);

}
void Fridge::deal_back()
{
    this->show();
}
void Fridge::on_warning_clicked()
{
    emit send_to_warning(get_mac, get_name);
    qDebug() << "here"<<get_mac << get_name;
    fridge_warning->show();
    this->hide();
}


void Fridge::set_current(QString status_fresh, QString status_temp)
{
    ui->current_fresh->setText(status_fresh);
    ui->current_temp->setText(status_temp);
}
void Fridge::deal_mac(QString mac)
{
    get_mac = mac;
}
void Fridge::deal_acc_name(QString name)
{
    get_name = name;
}

void Fridge::deal_set_signal(QString set, QString set_current)
{
    if (set == "fridge_fresh")
    {
        ui->current_fresh->setText(set_current);
    }
    if (set == "fridge_temp") {
        //qDebug() << "setMode";
        ui->current_temp->setText(set_current);
    }
}
