#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    bookDisplayWidget.reset(new BookDisplayWidget(nullptr));
    bookDisplayWidget->setAttribute(Qt::WA_DeleteOnClose);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_inBtn_clicked()
{
    bookDisplayWidget->show();

    this->close();
}

