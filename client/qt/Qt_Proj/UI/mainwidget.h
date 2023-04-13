#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include "logwidget.h"
#include "regwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWidget; }
QT_END_NAMESPACE

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();
    RegWidget reg;
    LogWidget log;
private slots:
    void on_Register_clicked();

    void on_Login_clicked();

    void on_Quit_clicked();

    void deal_regclose();
    void deal_logclose();

private:
    Ui::MainWidget *ui;
};
#endif // MAINWIDGET_H
