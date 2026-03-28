#include "../Inc/mainwindow.h"
#include "ui_mainwindow.h"
#include "../Inc/uart_handler.h"
#include "../Inc/data_manager.h"
#include "../Inc/can_bridge.h"
#include "../Inc/can_processor.h"
#include "../Inc/mcap_logger.h"
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , updateTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Start CAN Bridge and Processor
    startCanBridge();
    startCanProcessor();

    // Start MCAP Logger with Foxglove WebSocket streaming
    // Options:
    // 1. Stream only (no file): startMcapLogger("", 100, true);
    // 2. File + stream: startMcapLogger("/tmp/dashboard_data.mcap", 100, true);
    // 3. File only: startMcapLogger("/tmp/dashboard_data.mcap", 100, false);
    startMcapLogger("/tmp/dashboard_data.mcap", 100, true, "0.0.0.0", 8765);

    // UART Handler for steering wheel
    auto *uart = new UARTHandler(this);
    connect(uart, &UARTHandler::modeChanged, this, [this](QString mode){
        ui->suspensionModeLabel->setText(mode);
    });
    uart->connectPort("/dev/ttyAMA0");

    // Setup update timer (30Hz for smooth UI)
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateDisplay);
    updateTimer->start(33); // ~30Hz (33ms interval)
}

MainWindow::~MainWindow()
{
    // Stop all threads
    stopCanProcessor();
    stopCanBridge();
    stopMcapLogger();

    delete ui;
}

void MainWindow::updateDisplay() {
    // Get latest data from DataManager
    test::Data data = DataManager::getInstance().getLatestData();

    // Update dashboard sensors
    ui->tach_value->setText(QString::number(data.tach()));

    // Add more UI updates as you create the widgets in Qt Designer:
    // ui->speedo_value->setText(QString::number(data.speedo()));
    // ui->temp_value->setText(QString::number(data.temp()));
    // ui->fuel_value->setText(QString::number(data.fuel()));

    // ECU data (example for front left)
    // ui->fl_travel_value->setText(QString::number(data.ecu_fl().travel()));
    // ui->fl_strain_l_value->setText(QString::number(data.ecu_fl().strain_l()));
    // ui->fl_strain_r_value->setText(QString::number(data.ecu_fl().strain_r()));

    // GPS data
    // if (data.location().has_fix()) {
    //     ui->gps_lat->setText(QString::number(data.location().latitude(), 'f', 6));
    //     ui->gps_lon->setText(QString::number(data.location().longitude(), 'f', 6));
    //     ui->gps_speed->setText(QString::number(data.location().gps_speed()));
    // }
}