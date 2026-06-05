#include "data_manager.h"
#include "baja.pb.h"
#include <QMetaObject>
#include <QTimer>

// ─── Pimpl ────────────────────────────────────────────────────────────────────

struct DataManager::Impl {
    ubcbaja::Data masterData;
};

// ─── Singleton ────────────────────────────────────────────────────────────────

DataManager::DataManager() : impl_(new Impl()) {
    watchdog_ = new QTimer(this);
    watchdog_->setInterval(2000);
    watchdog_->setSingleShot(false);
    connect(watchdog_, &QTimer::timeout, this, [this]() {
        if (!noSignal_) {
            noSignal_ = true;
            emit noSignalChanged(true);
        }
    });
    watchdog_->start();
}

DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

ubcbaja::Data DataManager::getLatestData() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return impl_->masterData;
}

// ─── Watchdog ─────────────────────────────────────────────────────────────────

void DataManager::kickWatchdog() {
    QMetaObject::invokeMethod(watchdog_, "start", Qt::QueuedConnection);
    if (noSignal_) {
        noSignal_ = false;
        emit noSignalChanged(false);
    }
}

// ─── Property Readers ─────────────────────────────────────────────────────────

bool  DataManager::noSignal()  const { return noSignal_; }
int   DataManager::tach()      const { std::lock_guard<std::mutex> l(dataMutex_); return static_cast<int>(impl_->masterData.tach()); }
int   DataManager::speed()     const { std::lock_guard<std::mutex> l(dataMutex_); return static_cast<int>(impl_->masterData.speedo()); }
int   DataManager::temp()      const { std::lock_guard<std::mutex> l(dataMutex_); return static_cast<int>(impl_->masterData.temp()); }
int   DataManager::fuel()      const { std::lock_guard<std::mutex> l(dataMutex_); return static_cast<int>(impl_->masterData.fuel()); }

float DataManager::latitude()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.location().latitude(); }
float DataManager::longitude() const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.location().longitude(); }
float DataManager::gpsSpeed()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.location().gps_speed(); }
bool  DataManager::hasFix()    const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.location().has_fix(); }

float DataManager::flTravel()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_fl().travel(); }
float DataManager::flAccelX()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_fl().accel().x(); }
float DataManager::flAccelY()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_fl().accel().y(); }
float DataManager::flAccelZ()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_fl().accel().z(); }

float DataManager::frTravel()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_fr().travel(); }
float DataManager::frAccelX()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_fr().accel().x(); }
float DataManager::frAccelY()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_fr().accel().y(); }
float DataManager::frAccelZ()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_fr().accel().z(); }

float DataManager::rlTravel()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_rl().travel(); }
float DataManager::rlAccelX()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_rl().accel().x(); }
float DataManager::rlAccelY()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_rl().accel().y(); }
float DataManager::rlAccelZ()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_rl().accel().z(); }

float DataManager::rrTravel()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_rr().travel(); }
float DataManager::rrAccelX()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_rr().accel().x(); }
float DataManager::rrAccelY()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_rr().accel().y(); }
float DataManager::rrAccelZ()  const { std::lock_guard<std::mutex> l(dataMutex_); return impl_->masterData.ecu_rr().accel().z(); }

// ─── Setters ──────────────────────────────────────────────────────────────────

void DataManager::setTach(uint32_t rpm) {
    kickWatchdog();
    { std::lock_guard<std::mutex> lock(dataMutex_); impl_->masterData.set_tach(rpm); }
    QMetaObject::invokeMethod(this, &DataManager::dataChanged, Qt::QueuedConnection);
}

void DataManager::setSpeed(uint32_t speed) {
    kickWatchdog();
    { std::lock_guard<std::mutex> lock(dataMutex_); impl_->masterData.set_speedo(speed); }
    QMetaObject::invokeMethod(this, &DataManager::dataChanged, Qt::QueuedConnection);
}

void DataManager::setTemp(uint32_t temp) {
    kickWatchdog();
    { std::lock_guard<std::mutex> lock(dataMutex_); impl_->masterData.set_temp(temp); }
    QMetaObject::invokeMethod(this, &DataManager::dataChanged, Qt::QueuedConnection);
}

void DataManager::setFuel(uint32_t fuel) {
    kickWatchdog();
    { std::lock_guard<std::mutex> lock(dataMutex_); impl_->masterData.set_fuel(fuel); }
    QMetaObject::invokeMethod(this, &DataManager::dataChanged, Qt::QueuedConnection);
}

// ─── ECU Helpers ──────────────────────────────────────────────────────────────

static ubcbaja::ECU* getEcu(ubcbaja::Data& data, DataManager::EcuPosition pos) {
    switch (pos) {
    case DataManager::FRONT_LEFT:  return data.mutable_ecu_fl();
    case DataManager::FRONT_RIGHT: return data.mutable_ecu_fr();
    case DataManager::REAR_LEFT:   return data.mutable_ecu_rl();
    case DataManager::REAR_RIGHT:  return data.mutable_ecu_rr();
    default:                       return nullptr;
    }
}

// ─── ECU Setters ──────────────────────────────────────────────────────────────

void DataManager::setEcuTravel(EcuPosition pos, float travel) {
    kickWatchdog();
    { std::lock_guard<std::mutex> lock(dataMutex_); if (auto* ecu = getEcu(impl_->masterData, pos)) ecu->set_travel(travel); }
    QMetaObject::invokeMethod(this, &DataManager::dataChanged, Qt::QueuedConnection);
}

void DataManager::setEcuStrain(EcuPosition pos, float strain_l, float strain_r) {
    kickWatchdog();
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        if (auto* ecu = getEcu(impl_->masterData, pos)) {
            if (strain_l != 0.0f) ecu->set_strain_l(strain_l);
            if (strain_r != 0.0f) ecu->set_strain_r(strain_r);
        }
    }
    QMetaObject::invokeMethod(this, &DataManager::dataChanged, Qt::QueuedConnection);
}

void DataManager::setEcuAccel(EcuPosition pos, float x, float y, float z) {
    kickWatchdog();
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        if (auto* ecu = getEcu(impl_->masterData, pos)) {
            auto* accel = ecu->mutable_accel();
            accel->set_x(x);
            accel->set_y(y);
            accel->set_z(z);
        }
    }
    QMetaObject::invokeMethod(this, &DataManager::dataChanged, Qt::QueuedConnection);
}

void DataManager::setEcuGyro(EcuPosition pos, float x, float y, float z) {
    kickWatchdog();
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        if (auto* ecu = getEcu(impl_->masterData, pos)) {
            auto* gyro = ecu->mutable_gyro();
            gyro->set_x(x);
            gyro->set_y(y);
            gyro->set_z(z);
        }
    }
    QMetaObject::invokeMethod(this, &DataManager::dataChanged, Qt::QueuedConnection);
}

// ─── GPS ──────────────────────────────────────────────────────────────────────

void DataManager::setGps(float latitude, float longitude, float speed, bool has_fix) {
    kickWatchdog();
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        auto* gps = impl_->masterData.mutable_location();
        gps->set_latitude(latitude);
        gps->set_longitude(longitude);
        gps->set_gps_speed(speed);
        gps->set_has_fix(has_fix);
    }
    QMetaObject::invokeMethod(this, &DataManager::dataChanged, Qt::QueuedConnection);
}