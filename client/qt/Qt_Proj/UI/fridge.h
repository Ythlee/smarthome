#ifndef FRIDGE_H
#define FRIDGE_H

#include <QWidget>
#include "setdialog.h"
#include "fridgewarning.h"
#include "include/client.h"
#include "include/media.h"
namespace Ui {
class Fridge;
}

class Fridge : public QWidget
{
    Q_OBJECT

public:
    explicit Fridge(QWidget *parent = nullptr);
    ~Fridge();

private slots:
    void on_return_clicked();
    void on_quit_clicked();
    void set_current(QString, QString);
    void deal_mac(QString);
    void on_fresh_clicked();
    void on_temp_clicked();
    void on_warning_clicked();
    void video_clicked();
    void deal_set_signal(QString, QString);
    void deal_back();
    void deal_acc_name(QString);
signals:
    void fridge_return();
    void sendsignal(QString);
    void send_mac(QString);
    void send_to_warning(QString, QString);
private:
    Ui::Fridge *ui;
    SetDialog *setdialog;
    FridgeWarning *fridge_warning;
    QString get_mac, get_name;
};

#endif // FRIDGE_H
