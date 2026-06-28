#include "microphone.h"
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QTimer>
#include <QDebug>

Microphone::Microphone(QObject* parent) : QObject(parent) {
    QAudioFormat format;
    format.setSampleRate(m_sampleRate);
    format.setChannelCount(m_channels);
    format.setSampleFormat(QAudioFormat::Int16); // Sets standard PCM 16-bit format

    QAudioDevice info = QMediaDevices::defaultAudioInput();
    if (!info.isFormatSupported(format)) {
        qWarning() << "[Microphone] Default format not supported. Audio may sound distorted.";
    }

    m_audioSource = new QAudioSource(info, format, this);
}

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

void Microphone::processAudio() {
    if (!m_ioDevice) return;

    QByteArray data = m_ioDevice->readAll();
    if (data.size() > 0) {
        emit audioReady("/camera/audio", data, m_sampleRate, m_channels);
    }
}