#ifndef WARNING_H
#define WARNING_H

#include <QWidget>
#include "include/client.h"
#include <QDebug>
namespace Ui {
class Warning;
}

class Warning : public QWidget
{
    Q_OBJECT

public:
    explicit Warning(QWidget *parent = nullptr);
    ~Warning();
signals:
    void send_back();
private slots:
    void deal_recv(QString , QString );
    void on_back_clicked();
private:
    Ui::Warning *ui;
    QString get_name, get_mac;
};

#endif // WARNING_H
