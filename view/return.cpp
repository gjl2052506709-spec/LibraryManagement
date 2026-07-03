#include "return.h"
#include "ui_return.h"

ReturnAndBorrow::ReturnAndBorrow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReturnAndBorrow)
{
    ui->setupUi(this);
}

ReturnAndBorrow::~ReturnAndBorrow()
{
    delete ui;
}
