#include "ui_mainwindow.h"
#include "../Inc/mainwindow.h"
#include "../Inc/uart_handler.h"
#include "../Inc/data_manager.h"
#include "../Inc/can_bridge.h"
#include "../Inc/mcap_logger.h"
#include "../Inc/nrf24_handler.h"
#include "../Inc/camera_handler.h"
#include "../Inc/audio_handler.h"

// ── ADDED INCLUDES FOR QML ────────────────────────────────────────────────────
#include <QQuickWidget>
#include <QQuickItem>
#include <QUrl>

#include <QTimer>
#include <QApplication>
#include <csignal>
#include <QMovie>
#include <QLabel>
#include <chrono>
#include <ctime>
#include <thread>

// ── Signal handler ────────────────────────────────────────────────────────────
static void handleSignal(int) {
    stopAudio();
    stopCamera();
    stopMcapLogger();
    stopCanBridge();
    stopNrf24();
    QApplication::quit();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , updateTimer(new QTimer(this))
{
    ui->setupUi(this);

    std::signal(SIGINT,  handleSignal);
    std::signal(SIGTERM, handleSignal);

    startCanBridge();
    startNrf24();

    connect(this, &MainWindow::newFrameReceived,
            this, &MainWindow::updateCameraDisplay,
            Qt::QueuedConnection);

    // ── SPEEDOMETER GAUGE SETUP ───────────────────────────────────────────────
    ui->speedometerWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->speedometerWidget->setAttribute(Qt::WA_AlwaysStackOnTop);
    ui->speedometerWidget->setClearColor(Qt::transparent);
    ui->speedometerWidget->setSource(QUrl(QStringLiteral("qrc:/QML/mainwindow.qml")));

    // --- Speedometer Setup ---
    if (QQuickItem* speedoRoot = ui->speedometerWidget->rootObject()) {
        speedoRoot->setProperty("unitText", "KM/H");
        speedoRoot->setProperty("maxValue", 60);

        // Speedometer gets the "Classic" Nyan look
        speedoRoot->setProperty("gifSource", "qrc:/Images/original.gif");
        speedoRoot->setProperty("gifWidth", 120);
        speedoRoot->setProperty("gifHeight", 74);

        QStringList speedoColors = {"#FF0000", "#FFaa00", "#33FF00", "#00ffee","#9c33ff", "#cc00ff"};
        speedoRoot->setProperty("rainbowColors", QVariant::fromValue(speedoColors));
    }



    // ── TACHOMETER GAUGE SETUP ────────────────────────────────────────────────
    ui->tachometerWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->tachometerWidget->setAttribute(Qt::WA_AlwaysStackOnTop);
    ui->tachometerWidget->setClearColor(Qt::transparent);
    ui->tachometerWidget->setSource(QUrl(QStringLiteral("qrc:/QML/mainwindow.qml")));

    // --- Tachometer Setup ---
    if (QQuickItem* tachRoot = ui->tachometerWidget->rootObject()) {
        tachRoot->setProperty("unitText", "RPM");
        tachRoot->setProperty("maxValue", 4000);

        // Tachometer gets a "Gamer" Neon look
        tachRoot->setProperty("gifSource", "qrc:/Images/gb.gif");
        tachRoot->setProperty("gifWidth", 120);
        tachRoot->setProperty("gifHeight", 74);

        // Different color set for the Tach
        QStringList tachColors = {"#19353b", "#204b3b", "#4b776f", "#445244", "#4b7644", "#9eb94e"};
        tachRoot->setProperty("rainbowColors", QVariant::fromValue(tachColors));
    }

    // 1. Start the camera (Restored)
    startCamera("/dev/video0", [this](const uint8_t* data, size_t size) {
        if (is_ui_busy.load()) return;
        is_ui_busy.store(true);
        QByteArray arr(reinterpret_cast<const char*>(data), static_cast<int>(size));
        emit newFrameReceived(arr);
    });

    // 2. Start Audio on a delayed background thread
    std::thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        printf("Main: 2 seconds passed. Starting audio now...\n");
        startAudio("plughw:U0x46d0x825,0");
    }).detach();


    // Nyan Cat
    QMovie *movie = new QMovie(":/Images/nyanbg.gif", QByteArray(), this);
    ui->nyanbg->setMovie(movie);
    movie->start();

    // Timestamped MCAP filename
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    char filename[128];
    std::strftime(filename, sizeof(filename),
                  "/home/ubcbaja/firmware/logs/foxglove/%Y%m%d_%H%M%S.mcap",
                  std::localtime(&t));

    startMcapLogger(filename, 100, true, "0.0.0.0", 8765);

    // UART Handler
    auto *uart = new UARTHandler(this);
    connect(uart, &UARTHandler::modeChanged, this, [this](QString mode) {
        ui->suspensionModeLabel->setText(mode);
    });
    uart->connectPort("/dev/ttyAMA0");

    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateDisplay);
    updateTimer->start(33);
}

MainWindow::~MainWindow()
{
    stopAudio();
    stopCamera();
    stopCanBridge();
    stopMcapLogger();
    stopNrf24();
    delete ui;
}

void MainWindow::updateDisplay() {
    ubcbaja::Data data = DataManager::getInstance().getLatestData();

    // Standard Widget update (Optional: Keep this if you still have the old text label)
    // ui->tach_value->setText(QString::number(data.tach()));

    // ── QML GAUGE UPDATES ─────────────────────────────────────────────────────

    // Update Speedometer
    if (QQuickItem* speedoRoot = ui->speedometerWidget->rootObject()) {
        // Assuming your 'Data' class has a speed() method.
        // If it's called something else in your protobuf, adjust accordingly.
        speedoRoot->setProperty("value", data.speedo());
    }

    // Update Tachometer
    if (QQuickItem* tachRoot = ui->tachometerWidget->rootObject()) {
        tachRoot->setProperty("value", data.tach());
    }
}

void MainWindow::updateCameraDisplay(const QByteArray& frameData) {
    QPixmap pixmap;
    if (pixmap.loadFromData(frameData, "JPG")) {
        ui->cameraLabel->setPixmap(pixmap.scaled(ui->cameraLabel->size(),
                                                 Qt::KeepAspectRatio,
                                                 Qt::FastTransformation));
    }
    is_ui_busy.store(false);
}