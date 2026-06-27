#pragma once

#include <QObject>
#include <QAudioSource>
#include <QByteArray>

class Microphone : public QObject {
    Q_OBJECT
public:
    explicit Microphone(QObject* parent = nullptr);
    void start();

    signals:
        void audioReady(const QString& topic, const QByteArray& pcmData, int sampleRate, int channels);

private slots:
    void processAudio();

private:
    QAudioSource* m_audioSource = nullptr;
    QIODevice* m_ioDevice = nullptr;
    int m_sampleRate = 16000; // 16kHz is great for voice/engine noise and saves bandwidth
    int m_channels = 1;       // Mono audio
};