#ifndef AIRCONDITION_H
#define AIRCONDITION_H

#include <QWidget>
#include <QDialog>
#include "warning.h"
#include "aironoff.h"
#include "setdialog.h"
namespace Ui {
class Aircondition;
}

class Aircondition : public QWidget
{
    Q_OBJECT

public:
    explicit Aircondition(QWidget *parent = nullptr);
    ~Aircondition();

private slots:
    void on_return_clicked();
    void on_quit_clicked();
    void on_on_off_clicked();
    void on_mode_clicked();
    void on_temp_clicked();
    void on_wind_clicked();
    void on_warning_clicked();
    void set_current(QString, QString, QString, QString);
    void deal_mac(QString);
    void deal_acc_name(QString);
    void deal_set_signal(QString, QString);
    void deal_back();
signals:
    void aircondition_return();
    void sendsignal(QString);
    void send_mac(QString);
    void send_to_warning(QString, QString);
private:
    Ui::Aircondition *ui;
    //QDialog *dia_onoff;
    AirOnOff *aironoff;
    SetDialog *setdialog;
    Warning *warning;
    QString get_mac, get_name;
};

#endif // AIRCONDITION_H
