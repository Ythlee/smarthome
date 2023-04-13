#include "regdialog.h"
#include "ui_regdialog.h"

RegDialog::RegDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegDialog)
{
    this->setStyleSheet("QPushButton{font:bold}");
    ui->setupUi(this);
}

RegDialog::~RegDialog()
{
    delete ui;
}

void RegDialog::on_pushButton_clicked()
{
    this->close();
}

void RegDialog::deal_signal(QString str) {
    ui->label->setText(str);
}
