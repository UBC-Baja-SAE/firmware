#include "microphone.h"
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QTimer> // <-- ADD THIS
#include <QDebug>

// ... (Your constructor stays the same) ...

void Microphone::start() {
    // 1. Check if the Linux kernel has actually enumerated any audio devices yet
    if (QMediaDevices::audioInputs().isEmpty()) {
        qWarning() << "[Microphone] No I2S audio device found yet. Retrying in 2 seconds...";

        // 2. Setup a single-shot timer to try calling start() again in 2000ms
        QTimer::singleShot(2000, this, &Microphone::start);
        return;
    }

    // 3. Devices exist! Safe to initialize.
    m_ioDevice = m_audioSource->start();
    if (m_ioDevice) {
        connect(m_ioDevice, &QIODevice::readyRead, this, &Microphone::processAudio);
        qInfo() << "[Microphone] Started capturing live audio";
    } else {
        qWarning() << "[Microphone] Failed to open audio hardware";
    }
}