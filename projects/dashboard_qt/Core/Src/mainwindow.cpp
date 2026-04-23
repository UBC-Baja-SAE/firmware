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
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QMovie>
#include <QLabel>
#include <csignal>
#include <chrono>
#include <ctime>
#include <thread>
#include <unistd.h>

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

    if (QQuickItem* speedoRoot = ui->speedometerWidget->rootObject()) {
        speedoRoot->setProperty("unitText", "KM/H");
        speedoRoot->setProperty("maxValue", 60);
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

    if (QQuickItem* tachRoot = ui->tachometerWidget->rootObject()) {
        tachRoot->setProperty("unitText", "RPM");
        tachRoot->setProperty("maxValue", 4000);
        tachRoot->setProperty("gifSource", "qrc:/Images/gb.gif");
        tachRoot->setProperty("gifWidth", 120);
        tachRoot->setProperty("gifHeight", 74);
        QStringList tachColors = {"#19353b", "#204b3b", "#4b776f", "#445244", "#4b7644", "#9eb94e"};
        tachRoot->setProperty("rainbowColors", QVariant::fromValue(tachColors));
    }

    // ── CAMERA ────────────────────────────────────────────────────────────────
    startCamera("/dev/video0", [this](const uint8_t* data, size_t size) {
        if (is_ui_busy.load()) return;
        is_ui_busy.store(true);
        QByteArray arr(reinterpret_cast<const char*>(data), static_cast<int>(size));
        emit newFrameReceived(arr);
    });

    // ── AUDIO (delayed) ───────────────────────────────────────────────────────
    std::thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        printf("Main: 2 seconds passed. Starting audio now...\n");
        startAudio("plughw:U0x46d0x825,0");
    }).detach();

    // ── NYAN CAT BACKGROUND ───────────────────────────────────────────────────
    QMovie *movie = new QMovie(":/Images/nyanbg.gif", QByteArray(), this);
    ui->nyanbg->setMovie(movie);
    movie->start();

    // ── MCAP LOGGER ───────────────────────────────────────────────────────────
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    char filename[128];
    std::strftime(filename, sizeof(filename),
                  "/home/ubcbaja/firmware/logs/foxglove/%Y%m%d_%H%M%S.mcap",
                  std::localtime(&t));
    startMcapLogger(filename, 100, false, "0.0.0.0", 8765);

    // ── UART HANDLER ──────────────────────────────────────────────────────────
    auto *uart = new UARTHandler(this);
    connect(uart, &UARTHandler::modeChanged, this, [this](QString mode) {
        ui->suspensionModeLabel->setText(mode);
        // Page navigation commands
        int pageCount = ui->stackedWidget->count();
        if (mode == "PAGE_NEXT" && currentIndex + 1 < pageCount) slideTo(currentIndex + 1, true);
        if (mode == "PAGE_PREV" && currentIndex - 1 >= 0)        slideTo(currentIndex - 1, false);
    });
    uart->connectPort("/tmp/ttyV0"); //temp for testing

    // ── UPDATE TIMER ──────────────────────────────────────────────────────────
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

void MainWindow::updateDisplay()
{
    ubcbaja::Data data = DataManager::getInstance().getLatestData();

    ui->tach_value->setText(QString::number(data.tach()));
    ui->speedo_value->setText(QString::number(data.speedo()));

    if (QQuickItem* speedoRoot = ui->speedometerWidget->rootObject())
        speedoRoot->setProperty("value", data.speedo());

    if (QQuickItem* tachRoot = ui->tachometerWidget->rootObject())
        tachRoot->setProperty("value", data.tach());
}

void MainWindow::updateCameraDisplay(const QByteArray& frameData)
{
    QPixmap pixmap;
    if (pixmap.loadFromData(frameData, "JPG")) {
        ui->cameraLabel->setPixmap(pixmap.scaled(ui->cameraLabel->size(),
                                                 Qt::KeepAspectRatio,
                                                 Qt::FastTransformation));
    }
    is_ui_busy.store(false);
}

void MainWindow::slideTo(int newIndex, bool slideLeft)
{
    if (m_animating) return;

    QStackedWidget* stack = ui->stackedWidget;

    int oldIndex = stack->currentIndex();
    if (oldIndex == newIndex) return;

    m_animating = true;

    QWidget* oldPage = stack->currentWidget();
    stack->setCurrentIndex(newIndex);
    QWidget* newPage = stack->currentWidget();

    int dx = slideLeft ? stack->width() : -stack->width();

    newPage->move(dx, 0);
    newPage->show();

    auto* oldAnim = new QPropertyAnimation(oldPage, "pos", this);
    oldAnim->setDuration(350);
    oldAnim->setStartValue(oldPage->pos());
    oldAnim->setEndValue(QPoint(-dx, 0));
    oldAnim->setEasingCurve(QEasingCurve::InOutCubic);

    auto* newAnim = new QPropertyAnimation(newPage, "pos", this);
    newAnim->setDuration(350);
    newAnim->setStartValue(QPoint(dx, 0));
    newAnim->setEndValue(QPoint(0, 0));
    newAnim->setEasingCurve(QEasingCurve::InOutCubic);

    auto* group = new QParallelAnimationGroup(this);
    group->addAnimation(oldAnim);
    group->addAnimation(newAnim);

    connect(group, &QParallelAnimationGroup::finished, this, [this, oldPage, newIndex, group]() {
        oldPage->move(0, 0);
        currentIndex = newIndex;
        m_animating = false;
        group->deleteLater();
    });

    group->start();
}