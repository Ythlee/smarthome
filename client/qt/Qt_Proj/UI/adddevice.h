#ifndef ADDDEVICE_H
#define ADDDEVICE_H

#include <QWidget>
#include "include/client.h"
namespace Ui {
class AddDevice;
}

class AddDevice : public QWidget
{
    Q_OBJECT

public:
    explicit AddDevice(QWidget *parent = nullptr);
    ~AddDevice();
    QString str;
private slots:
    void on_cancel_clicked();
    void send_name(QString);
signals:
    void cancel_singal();
    void deal_edit_clear();
    void device_name(QString);
private:
    Ui::AddDevice *ui;
};

#endif // ADDDEVICE_H
