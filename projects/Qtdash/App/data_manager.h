#pragma once
#include <QObject>
#include <QTimer>
#include <mutex>
#include <cstdint>

namespace ubcbaja { class Data; }

class DataManager : public QObject {
    Q_OBJECT

    // ── Powertrain ────────────────────────────────────────────────────────────
    Q_PROPERTY(int tach  READ tach  NOTIFY dataChanged)
    Q_PROPERTY(int speed READ speed NOTIFY dataChanged)
    Q_PROPERTY(int temp  READ temp  NOTIFY dataChanged)
    Q_PROPERTY(int fuel  READ fuel  NOTIFY dataChanged)

    // ── GPS ───────────────────────────────────────────────────────────────────
    Q_PROPERTY(float latitude  READ latitude  NOTIFY dataChanged)
    Q_PROPERTY(float longitude READ longitude NOTIFY dataChanged)
    Q_PROPERTY(float gpsSpeed  READ gpsSpeed  NOTIFY dataChanged)
    Q_PROPERTY(bool  hasFix    READ hasFix    NOTIFY dataChanged)

    // ── Front Left ECU ────────────────────────────────────────────────────────
    Q_PROPERTY(float flTravel READ flTravel NOTIFY dataChanged)
    Q_PROPERTY(float flAccelX READ flAccelX NOTIFY dataChanged)
    Q_PROPERTY(float flAccelY READ flAccelY NOTIFY dataChanged)
    Q_PROPERTY(float flAccelZ READ flAccelZ NOTIFY dataChanged)

    // ── Front Right ECU ───────────────────────────────────────────────────────
    Q_PROPERTY(float frTravel READ frTravel NOTIFY dataChanged)
    Q_PROPERTY(float frAccelX READ frAccelX NOTIFY dataChanged)
    Q_PROPERTY(float frAccelY READ frAccelY NOTIFY dataChanged)
    Q_PROPERTY(float frAccelZ READ frAccelZ NOTIFY dataChanged)

    // ── Rear Left ECU ─────────────────────────────────────────────────────────
    Q_PROPERTY(float rlTravel READ rlTravel NOTIFY dataChanged)
    Q_PROPERTY(float rlAccelX READ rlAccelX NOTIFY dataChanged)
    Q_PROPERTY(float rlAccelY READ rlAccelY NOTIFY dataChanged)
    Q_PROPERTY(float rlAccelZ READ rlAccelZ NOTIFY dataChanged)

    // ── Rear Right ECU ────────────────────────────────────────────────────────
    Q_PROPERTY(float rrTravel READ rrTravel NOTIFY dataChanged)
    Q_PROPERTY(float rrAccelX READ rrAccelX NOTIFY dataChanged)
    Q_PROPERTY(float rrAccelY READ rrAccelY NOTIFY dataChanged)
    Q_PROPERTY(float rrAccelZ READ rrAccelZ NOTIFY dataChanged)

    // ── CAN signal watchdog ───────────────────────────────────────────────────
    Q_PROPERTY(bool noSignal READ noSignal NOTIFY noSignalChanged)

public:
    enum EcuPosition { FRONT_LEFT, FRONT_RIGHT, REAR_LEFT, REAR_RIGHT };
    Q_ENUM(EcuPosition)

    static DataManager& getInstance();

    ubcbaja::Data getLatestData();

    // ── Property readers ──────────────────────────────────────────────────────
    int   tach()      const;
    int   speed()     const;
    int   temp()      const;
    int   fuel()      const;
    float latitude()  const;
    float longitude() const;
    float gpsSpeed()  const;
    bool  hasFix()    const;
    float flTravel()  const;
    float flAccelX()  const;
    float flAccelY()  const;
    float flAccelZ()  const;
    float frTravel()  const;
    float frAccelX()  const;
    float frAccelY()  const;
    float frAccelZ()  const;
    float rlTravel()  const;
    float rlAccelX()  const;
    float rlAccelY()  const;
    float rlAccelZ()  const;
    float rrTravel()  const;
    float rrAccelX()  const;
    float rrAccelY()  const;
    float rrAccelZ()  const;
    bool  noSignal()  const;

    // ── Setters ───────────────────────────────────────────────────────────────
    void setTach (uint32_t rpm);
    void setSpeed(uint32_t speed);
    void setTemp (uint32_t temp);
    void setFuel (uint32_t fuel);
    void setEcuTravel(EcuPosition pos, float travel);
    void setEcuStrain(EcuPosition pos, float strain_l, float strain_r);
    void setEcuAccel (EcuPosition pos, float x, float y, float z);
    void setEcuGyro  (EcuPosition pos, float x, float y, float z);
    void setGps(float latitude, float longitude, float speed, bool has_fix);

signals:
    void dataChanged();
    void noSignalChanged(bool noSignal);

private:
    DataManager();
    ~DataManager() = default;
    DataManager(const DataManager&)            = delete;
    DataManager& operator=(const DataManager&) = delete;

    void kickWatchdog();

    struct Impl;
    Impl*       impl_;
    mutable std::mutex dataMutex_;
    QTimer*     watchdog_  = nullptr;
    bool        noSignal_  = true;
};