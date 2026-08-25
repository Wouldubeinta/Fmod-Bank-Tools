#include "about.h"
#include "ui_about.h"

About::About(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::About)
{
    ui->setupUi(this);
    QString version = APP_VERSION;
    ui->version_label->setText(version);
}

About::~About()
{
    delete ui;
}

void About::on_close_PushButton_clicked()
{
    this->close();
}

