#include "about.h"
#include "ui_about.h"

About::About(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::About)
{
    ui->setupUi(this);
    ui->version_label->setText(APP_VERSION);
}

About::~About()
{
    delete ui;
}

void About::on_close_PushButton_clicked()
{
    this->close();
}

