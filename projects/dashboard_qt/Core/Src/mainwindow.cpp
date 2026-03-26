#include "../Inc/mainwindow.h"
#include "ui_mainwindow.h"
#include "../Inc/uart_handler.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    // Example: Once you start adding UI elements in Qt Designer,
    // you will access them here like this:
    //ui->speedo_value->setText("12");

    //ui->tach_value->setText("56");

    int currentRpm = 7500;

    ui->tach_value->setText(QString::number(currentRpm));

    auto *uart = new UARTHandler(this);

    connect(uart, &UARTHandler::modeChanged, this, [this](QString mode){
        ui->suspensionModeLabel->setText(mode);
    });

    uart->connectPort("/dev/ttyAMA0");
}

MainWindow::~MainWindow()
{
    delete ui;
}