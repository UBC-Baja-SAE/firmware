#ifndef CONTROLS_H
#define CONTROLS_H

#include <QObject>
#include <QTimer>

// Forward declaration for hid_device so we don't need to include hidapi.h here
struct hid_device_;
typedef struct hid_device_ hid_device;
QTimer *m_reconnectTimer;

class Controls : public QObject {
    Q_OBJECT
public:
    explicit Controls(QObject *parent = nullptr);
    ~Controls();

    signals:
        // Emits JSON payload ready to be piped to Foxglove
        void foxglovePayloadReady(const QString& topic, const QByteArray& jsonPayload);

private slots:
    void pollEvents();
    void delayedInit();

private:
    QTimer* m_timer;
    hid_device* m_device = nullptr;
    uint16_t m_lastButtons = 0;

    void connectToWiimote();
    void activateMotionPlus();
    void setReportMode(uint8_t mode);
    void sendKeyEvent(int qtKey, bool pressed);
    void handleButtonChange(uint16_t currentButtons, uint16_t mask, int qtKey);
};



#endif // CONTROLS_H