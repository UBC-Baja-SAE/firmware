#include "ui_mainwindow.h"
#include "../Inc/mainwindow.h"
#include "../Inc/uart_handler.h"
#include "../Inc/data_manager.h"
#include "../Inc/can_bridge.h"
#include "../Inc/mcap_logger.h"
#include "../Inc/nrf24_handler.h"
#include <QTimer>
#include <QApplication>
#include <csignal>

// ── Signal handler — ensures clean shutdown on Ctrl+C ────────────────────────
// Must be a plain function, not a lambda, for std::signal compatibility
static void handleSignal(int) {
    stopMcapLogger();
    stopCanBridge();
    QApplication::quit();
    stopNrf24();

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , updateTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Register signal handler before starting any threads
    std::signal(SIGINT,  handleSignal);
    std::signal(SIGTERM, handleSignal); // also catches kill/systemd stop

    // Start CAN Bridge
    startCanBridge();

    startNrf24();

    // Start MCAP Logger with Foxglove WebSocket streaming

    // Generate timestamped filename
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    char filename[128];
    std::strftime(filename, sizeof(filename),
                  "/home/ubcbaja/firmware/logs/foxglove/%Y%m%d_%H%M%S.mcap",
                  std::localtime(&t));

    // File and Websocket
    // startMcapLogger(filename, 100, true, "0.0.0.0", 8765);

    // File only, no websocket
    startMcapLogger("/home/ubcbaja/firmware/logs/foxglove/dashboard_data.mcap",
                    100, false, "", 0);

    // UART Handler for steering wheel
    auto *uart = new UARTHandler(this);
    connect(uart, &UARTHandler::modeChanged, this, [this](QString mode) {
        ui->suspensionModeLabel->setText(mode);
    });
    uart->connectPort("/dev/ttyAMA0");

    // Setup update timer (~30Hz)
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateDisplay);
    updateTimer->start(33);
}

MainWindow::~MainWindow()
{
    // Normal shutdown path (window closed via UI)
    stopCanBridge();
    stopMcapLogger();
    stopNrf24();
    delete ui;
}

void MainWindow::updateDisplay() {
    test::Data data = DataManager::getInstance().getLatestData();

    ui->tach_value->setText(QString::number(data.tach()));

    // ui->speedo_value->setText(QString::number(data.speedo()));
    // ui->temp_value->setText(QString::number(data.temp()));
    // ui->fuel_value->setText(QString::number(data.fuel()));

    // ui->fl_travel_value->setText(QString::number(data.ecu_fl().travel()));
    // ui->fl_strain_l_value->setText(QString::number(data.ecu_fl().strain_l()));
    // ui->fl_strain_r_value->setText(QString::number(data.ecu_fl().strain_r()));

    // if (data.location().has_fix()) {
    //     ui->gps_lat->setText(QString::number(data.location().latitude(), 'f', 6));
    //     ui->gps_lon->setText(QString::number(data.location().longitude(), 'f', 6));
    //     ui->gps_speed->setText(QString::number(data.location().gps_speed()));
    // }
}


