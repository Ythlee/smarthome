#ifndef REGWIDGET_H
#define REGWIDGET_H
#include "include/client.h"
#include <QWidget>
#include "regdialog.h"
namespace Ui {
class RegWidget;
}

class RegWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RegWidget(QWidget *parent = nullptr);
    ~RegWidget();
    RegDialog regdialog;
    QString input_name;
    QString input_password;

signals:
    void reg_close_singal();
    void res_signal(QString);
private slots:
    void on_quit_clicked();

private:
    Ui::RegWidget *ui;
};

#endif // REGWIDGET_H
