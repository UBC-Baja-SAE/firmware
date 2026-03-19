#include "../Inc/mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    // Example: Once you start adding UI elements in Qt Designer,
    // you will access them here like this:
    //ui->speedo_value->setText("12");

    //ui->tach_value->setText("56");
}

MainWindow::~MainWindow()
{
    delete ui;
}