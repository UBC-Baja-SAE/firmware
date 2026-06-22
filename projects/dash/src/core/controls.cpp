#include "controls.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QWindow>
#include <hidapi.h>

// Nintendo Vendor ID
#define VENDOR_ID 0x057e
// Wiimote Product IDs (Original and TR/MotionPlus Built-In)
#define PRODUCT_ID_WIIMOTE 0x0306
#define PRODUCT_ID_WIIMOTE_PLUS 0x0330

// Wiimote Button Bitmasks for bytes 1 & 2
#define WIIMOTE_BUTTON_LEFT  0x0001
#define WIIMOTE_BUTTON_RIGHT 0x0002
#define WIIMOTE_BUTTON_DOWN  0x0004
#define WIIMOTE_BUTTON_UP    0x0008
#define WIIMOTE_BUTTON_PLUS  0x0010
#define WIIMOTE_BUTTON_TWO   0x0100
#define WIIMOTE_BUTTON_ONE   0x0200
#define WIIMOTE_BUTTON_B     0x0400
#define WIIMOTE_BUTTON_A     0x0800
#define WIIMOTE_BUTTON_MINUS 0x1000
#define WIIMOTE_BUTTON_HOME  0x8000

Controls::Controls(QObject *parent) : QObject(parent), m_device(nullptr), m_lastButtons(0) {
    if (hid_init() != 0) {
        qWarning() << "[Controls] Failed to initialize hidapi.";
        return;
    }

    connectToWiimote();

    // Setup polling timer to fetch HID packets at ~60Hz
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Controls::pollEvents);
    m_timer->start(16);
}

Controls::~Controls() {
    if (m_device) {
        hid_close(m_device);
    }
    hid_exit();
}

void Controls::connectToWiimote() {
    // Try Wiimote with built-in MotionPlus first
    m_device = hid_open(VENDOR_ID, PRODUCT_ID_WIIMOTE_PLUS, nullptr);
    if (!m_device) {
        // Fallback to original Wiimote
        m_device = hid_open(VENDOR_ID, PRODUCT_ID_WIIMOTE, nullptr);
    }

    if (m_device) {
        qInfo() << "[Controls] Wiimote connected via hidapi.";
        hid_set_nonblocking(m_device, 1);

        // Turn on Player 1 LED to signify connection
        unsigned char ledCmd[2] = {0x11, 0x10};
        hid_write(m_device, ledCmd, sizeof(ledCmd));

        // Use a short delay before activating extensions (Wiimote needs a moment)
        QTimer::singleShot(200, this, &Controls::delayedInit);
    } else {
        qWarning() << "[Controls] Could not find a connected Wiimote. Ensure it is paired.";
    }
}

void Controls::delayedInit() {
    activateMotionPlus();
    // Switch to Report 0x35 (Buttons + Accel + 16-byte Extension [Gyro])
    setReportMode(0x35);
}

void Controls::activateMotionPlus() {
    if (!m_device) return;

    // Complex initialization sequence to activate the MotionPlus extension block
    // 1. Write 0x55 to register 0xA600F0
    unsigned char cmd1[] = {0x16, 0x04, 0xA6, 0x00, 0xF0, 0x01, 0x55};
    hid_write(m_device, cmd1, sizeof(cmd1));

    // 2. Write 0x00 to register 0xA600FB
    unsigned char cmd2[] = {0x16, 0x04, 0xA6, 0x00, 0xFB, 0x01, 0x00};
    hid_write(m_device, cmd2, sizeof(cmd2));

    qInfo() << "[Controls] Sent MotionPlus initialization handshake.";
}

void Controls::setReportMode(uint8_t mode) {
    if (!m_device) return;
    // Format: [Report ID: 0x12] [Continuous: 0x04] [Report Type]
    unsigned char cmd[3] = {0x12, 0x04, mode};
    hid_write(m_device, cmd, sizeof(cmd));
    qInfo() << "[Controls] Requested Data Report Mode: 0x" << QString::number(mode, 16);
}

void Controls::sendKeyEvent(int qtKey, bool pressed) {
    // Inject hardware events directly into the QML engine
    QWindow *window = QGuiApplication::focusWindow();
    if (!window && !QGuiApplication::topLevelWindows().isEmpty()) {
        window = QGuiApplication::topLevelWindows().first();
    }
    if (!window) return;

    QKeyEvent* event = new QKeyEvent(
        pressed ? QEvent::KeyPress : QEvent::KeyRelease,
        qtKey,
        Qt::NoModifier
    );
    QCoreApplication::postEvent(window, event);
}

void Controls::handleButtonChange(uint16_t currentButtons, uint16_t mask, int qtKey) {
    bool wasPressed = (m_lastButtons & mask) != 0;
    bool isPressed = (currentButtons & mask) != 0;

    if (isPressed && !wasPressed) sendKeyEvent(qtKey, true);
    else if (!isPressed && wasPressed) sendKeyEvent(qtKey, false);
}

void Controls::pollEvents() {
    if (!m_device) return;

    unsigned char buf[64];
    int res = 0;

    // Drain all available HID packets from the Bluetooth queue
    while ((res = hid_read(m_device, buf, sizeof(buf))) > 0) {
        // We look for Report 0x31 (Accel only) or 0x35 (Accel + MotionPlus)
        if (buf[0] == 0x31 || buf[0] == 0x35) {

            // --- Parse Buttons ---
            uint16_t currentButtons = (buf[2] << 8) | buf[1];

            handleButtonChange(currentButtons, WIIMOTE_BUTTON_LEFT,  Qt::Key_Left);
            handleButtonChange(currentButtons, WIIMOTE_BUTTON_RIGHT, Qt::Key_Right);
            handleButtonChange(currentButtons, WIIMOTE_BUTTON_UP,    Qt::Key_Up);
            handleButtonChange(currentButtons, WIIMOTE_BUTTON_DOWN,  Qt::Key_Down);
            handleButtonChange(currentButtons, WIIMOTE_BUTTON_A,     Qt::Key_Enter);
            handleButtonChange(currentButtons, WIIMOTE_BUTTON_B,     Qt::Key_Escape);

            m_lastButtons = currentButtons;

            // --- Parse Accelerometer & Gyro ---
            QJsonObject imuData;

            // Accelerometer Raw bytes (0x80 is roughly 0g)
            imuData["accel_x"] = buf[3];
            imuData["accel_y"] = buf[4];
            imuData["accel_z"] = buf[5];

            // If we are in 0x35 mode, parse the extension bytes for Gyro
            if (buf[0] == 0x35) {
                // MotionPlus packs the 14-bit gyro data across 6 bytes (Bytes 6-11)
                // This does basic extraction of the raw rotation velocity values.
                int gyro_yaw   = buf[6] | ((buf[9] >> 2) << 8);
                int gyro_roll  = buf[7] | ((buf[10] >> 2) << 8);
                int gyro_pitch = buf[8] | ((buf[11] >> 2) << 8);

                imuData["gyro_yaw"]   = gyro_yaw;
                imuData["gyro_roll"]  = gyro_roll;
                imuData["gyro_pitch"] = gyro_pitch;
            }

            QJsonDocument doc(imuData);
            emit foxglovePayloadReady("wiimote/imu", doc.toJson(QJsonDocument::Compact));
        }
    }

    if (res < 0) {
        qWarning() << "[Controls] Disconnected or read error.";
        hid_close(m_device);
        m_device = nullptr;
    }
}