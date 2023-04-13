#include "mainwidget.h"
#include "ui_mainwidget.h"

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{

    this->setStyleSheet("QPushButton{font:bold}");
    connect(&reg, &RegWidget::reg_close_singal, this, &MainWidget::deal_regclose);
    connect(&log, &LogWidget::log_close_singal, this, &MainWidget::deal_logclose);
    ui->setupUi(this);
}

MainWidget::~MainWidget()
{
    delete ui;
}


void MainWidget::on_Register_clicked()
{
    
    
    this->close();
    reg.show();
}

void MainWidget::on_Login_clicked()
{
    this->close();
    log.show();
}

void MainWidget::on_Quit_clicked()
{
    this->close();
}

void MainWidget::deal_regclose()
{
    this->show();
}

void MainWidget::deal_logclose()
{
    this->show();
}
