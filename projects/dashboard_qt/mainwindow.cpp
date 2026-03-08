#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // This physically builds the UI on the screen based on your XML design
    ui->setupUi(this);

    // Example: Once you start adding UI elements in Qt Designer,
    // you will access them here like this:
    // ui->speedLabel->setText("0 mph");
}

MainWindow::~MainWindow()
{
    delete ui;
}//
// Created by Benjamin Friesen on 2026-03-07.
//