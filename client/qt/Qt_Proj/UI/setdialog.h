#ifndef SETDIALOG_H
#define SETDIALOG_H

#include <QDialog>
#include "include/client.h"
namespace Ui {
class SetDialog;
}

class SetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetDialog(QWidget *parent = nullptr);
    ~SetDialog();

private slots:
    void on_ok_clicked();
    void deal_sendsignal(QString);
    void deal_mac(QString);
signals:
    void num_singals(int);
    void set_signal(QString, QString);
    void set_frisignal(QString, QString);
private:
    Ui::SetDialog *ui;
    int num = 0;
    QString setwhich, get_mac;
};

#endif // SETDIALOG_H
