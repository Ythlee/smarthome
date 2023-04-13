#ifndef AIRONOFF_H
#define AIRONOFF_H

#include <QDialog>
#include "include/client.h"
namespace Ui {
class AirOnOff;
}

class AirOnOff : public QDialog
{
    Q_OBJECT

public:
    explicit AirOnOff(QWidget *parent = nullptr);
    ~AirOnOff();

signals:
    void set_signal(QString, QString);

private slots:
    void on_on_clicked();
    void on_off_clicked();
    void deal_mac(QString mac);
private:
    Ui::AirOnOff *ui;
    QString get_mac;
};

#endif // AIRONOFF_H
