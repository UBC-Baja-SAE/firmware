#include "wheel.h"

#include <QDebug>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

#ifdef __linux__
#include <gpiod.h>
#include <cerrno>
#include <cstring>
#endif

// ---------------------------------------------------------------------------
// WheelWorker
// ---------------------------------------------------------------------------

WheelWorker::WheelWorker(int pinA, int pinB, int countsPerRev, QObject* parent)
    : QObject(parent), m_pinA(pinA), m_pinB(pinB), m_countsPerRev(countsPerRev) {}

void WheelWorker::stop() {
    m_running.store(false);
}

void WheelWorker::zero() {
    m_zeroRequested.store(true);
}

void WheelWorker::emitAngle() {
    const long long counts = m_position.load();
    // x4 quadrature decoding -> 4 raw transitions per encoder line
    const double revs = static_cast<double>(counts) / (static_cast<double>(m_countsPerRev) * 4.0);
    const double angleDeg = revs * 360.0;

    const auto now = QDateTime::currentDateTimeUtc();
    QJsonObject ts;
    ts["sec"] = static_cast<qint64>(now.toSecsSinceEpoch());
    ts["nsec"] = static_cast<qint64>((now.toMSecsSinceEpoch() % 1000) * 1000000LL);

    QJsonObject obj;
    obj["timestamp"] = ts;
    obj["angle_deg"] = angleDeg;
    obj["raw_counts"] = static_cast<qint64>(counts);

    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    emit foxglovePayloadReady("/wheel/angle", payload);
}

#ifdef __linux__

QString WheelWorker::findRP1ChipPath() {
    // Scan for the RP1 chip instead of hardcoding gpiochip0/4 -- the index
    // isn't guaranteed across kernel/firmware versions.
    for (int i = 0; i < 8; ++i) {
        const QString path = QString("/dev/gpiochip%1").arg(i);
        if (!QFile::exists(path)) continue;

        gpiod_chip* chip = gpiod_chip_open(path.toStdString().c_str());
        if (!chip) continue;

        QString label;
        if (gpiod_chip_info* info = gpiod_chip_get_info(chip)) {
            label = QString::fromLatin1(gpiod_chip_info_get_label(info));
            gpiod_chip_info_free(info);
        }
        gpiod_chip_close(chip);

        if (label == "pinctrl-rp1") {
            return path;
        }
    }
    qWarning() << "[Wheel] Could not auto-detect RP1 gpiochip, falling back to gpiochip0";
    return "/dev/gpiochip0";
}

void WheelWorker::process() {
    const QString chipPath = findRP1ChipPath();
    qInfo() << "[Wheel] Using" << chipPath << "pins" << m_pinA << "/" << m_pinB;

    gpiod_chip* chip = gpiod_chip_open(chipPath.toStdString().c_str());
    if (!chip) {
        qWarning() << "[Wheel] Failed to open" << chipPath << ":" << strerror(errno);
        emit finished();
        return;
    }

    gpiod_line_settings* settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

    gpiod_line_config* lineCfg = gpiod_line_config_new();
    unsigned int offsets[2] = {static_cast<unsigned int>(m_pinA), static_cast<unsigned int>(m_pinB)};
    gpiod_line_config_add_line_settings(lineCfg, offsets, 2, settings);

    gpiod_request_config* reqCfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(reqCfg, "minimaldash-wheel");

    gpiod_line_request* request = gpiod_chip_request_lines(chip, reqCfg, lineCfg);

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineCfg);
    gpiod_request_config_free(reqCfg);
    gpiod_chip_close(chip); // request holds its own reference now

    if (!request) {
        qWarning() << "[Wheel] Failed to request GPIO lines:" << strerror(errno);
        emit finished();
        return;
    }

    // Seed initial A/B state so the first transition decodes correctly.
    const int valA = gpiod_line_request_get_value(request, m_pinA);
    const int valB = gpiod_line_request_get_value(request, m_pinB);
    uint8_t state = (valA == GPIOD_LINE_VALUE_ACTIVE ? 0b10 : 0) |
                     (valB == GPIOD_LINE_VALUE_ACTIVE ? 0b01 : 0);

    // Standard x4 quadrature decode table, indexed by (oldState<<2 | newState).
    // +1/-1 for valid single-bit transitions, 0 for invalid/double transitions.
    static const int8_t kQuadTable[16] = {
         0, -1,  1,  0,
         1,  0,  0, -1,
        -1,  0,  0,  1,
         0,  1, -1,  0
    };

    gpiod_edge_event_buffer* buf = gpiod_edge_event_buffer_new(4);
    constexpr qint64 kPublishIntervalMs = 20;      // 50 Hz telemetry rate
    constexpr int64_t kWaitTimeoutNs = 20'000'000LL; // 20 ms

    qint64 lastPublishMs = QDateTime::currentMSecsSinceEpoch();

    while (m_running.load()) {
        if (m_zeroRequested.exchange(false)) {
            m_position.store(0);
        }

        const int ret = gpiod_line_request_wait_edge_events(request, kWaitTimeoutNs);
        if (ret < 0) {
            qWarning() << "[Wheel] wait_edge_events error:" << strerror(errno);
            break;
        }

        if (ret > 0) {
            const int n = gpiod_line_request_read_edge_events(request, buf, 4);
            for (int i = 0; i < n; ++i) {
                gpiod_edge_event* event = gpiod_edge_event_buffer_get_event(buf, i);
                const unsigned int offset = gpiod_edge_event_get_line_offset(event);
                const bool rising = gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_RISING_EDGE;

                uint8_t newState = state;
                if (static_cast<int>(offset) == m_pinA) {
                    newState = rising ? (state | 0b10) : (state & ~0b10);
                } else if (static_cast<int>(offset) == m_pinB) {
                    newState = rising ? (state | 0b01) : (state & ~0b01);
                }

                const int delta = kQuadTable[((state & 0x3) << 2) | (newState & 0x3)];
                if (delta != 0) {
                    m_position.fetch_add(delta);
                } else if (newState != state) {
                    qWarning() << "[Wheel] Ambiguous quadrature transition (missed edge?)";
                }
                state = newState;
            }
        }

        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        if (nowMs - lastPublishMs >= kPublishIntervalMs) {
            emitAngle();
            lastPublishMs = nowMs;
        }
    }

    gpiod_edge_event_buffer_free(buf);
    gpiod_line_request_release(request);
    emit finished();
}

#else // !__linux__

// Desktop stub (Windows/macOS dev builds) -- no gpiod available. Keeps this
// class linkable when building the dashboard for desktop dev without hardware.
void WheelWorker::process() {
    qInfo() << "[Wheel] Non-Linux build: GPIO encoder disabled (stub).";
    while (m_running.load()) {
        if (m_zeroRequested.exchange(false)) {
            m_position.store(0);
        }
        QThread::msleep(20);
        emitAngle();
    }
    emit finished();
}

#endif

// ---------------------------------------------------------------------------
// Wheel
// ---------------------------------------------------------------------------

Wheel::Wheel(int pinA, int pinB, int countsPerRev, QObject* parent)
    : QObject(parent) {
    m_worker = new WheelWorker(pinA, pinB, countsPerRev);
    m_worker->moveToThread(&m_thread);

    connect(&m_thread, &QThread::started, m_worker, &WheelWorker::process);
    connect(m_worker, &WheelWorker::finished, &m_thread, &QThread::quit);
    connect(m_worker, &WheelWorker::foxglovePayloadReady, this, &Wheel::foxglovePayloadReady);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
}

Wheel::~Wheel() {
    stop();
}

void Wheel::start() {
    m_thread.start();
}

void Wheel::stop() {
    if (m_thread.isRunning()) {
        m_worker->stop();
        m_thread.quit();
        m_thread.wait(1000);
    }
}

void Wheel::zero() {
    m_worker->zero();
}