#ifndef FRIDGEWARNING_H
#define FRIDGEWARNING_H

#include <QWidget>
#include "include/client.h"
#include <QDebug>
namespace Ui {
class FridgeWarning;
}

class FridgeWarning : public QWidget
{
    Q_OBJECT

public:
    explicit FridgeWarning(QWidget *parent = nullptr);
    ~FridgeWarning();
signals:
    void send_back();
private slots:
    void deal_recv(QString , QString );
    void on_back_clicked();
private:
    Ui::FridgeWarning *ui;
    QString get_name, get_mac;
};

#endif // FRIDGEWARNING_H
