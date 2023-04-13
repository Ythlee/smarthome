#ifndef FIRSTWIDGET_H
#define FIRSTWIDGET_H

#include <QWidget>
#include "adddevice.h"
#include "aircondition.h"
#include "fridge.h"
#include "include/client.h"
namespace Ui {
class FirstWidget;
}

class FirstWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FirstWidget(QWidget *parent = nullptr);
    ~FirstWidget();
    //AddDevice addDevice;
    QString get_name;
    QList<QString> device;
private slots:
    void on_Quit_clicked();
    void on_Add_clicked();
    void on_Logout_clicked();
    void Choose_clicked();
    void setAccount(QString);
    void deal_devicename(QString);
    void deal_cancel();
    void deal_air_return();
    void deal_fridge_return();

signals:
    void logout_singal();
    void edit_clear_singal();
    void name_signal(QString);
    void send_mac(QString);
    void send_acc_name(QString);
    void set_current_signals(QString, QString, QString, QString);
    void set_current_signals(QString, QString);
private:
    Ui::FirstWidget *ui;
    AddDevice *addDevice;
    Aircondition *aircondition;
    Fridge *fridge;
};

#endif // FIRSTWIDGET_H
