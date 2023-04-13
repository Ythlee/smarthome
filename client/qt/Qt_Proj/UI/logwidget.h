#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include "regdialog.h"
#include "firstwidget.h"
#include "include/client.h"
namespace Ui {
class LogWidget;
}

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);
    ~LogWidget();
    RegDialog logdialog;
    FirstWidget firstwidget;
signals:
    void log_close_singal();
    void singal(QString);
    void res_signal(QString);

private slots:
    void on_quit_clicked();
    void deal_logout();

private:
    Ui::LogWidget *ui;
};

#endif // LOGWIDGET_H
