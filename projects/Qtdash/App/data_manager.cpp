#include "data_manager.h"

// Generated protobuf header — only included here, not in the .h
#include "baja.pb.h"

// ─── Pimpl ────────────────────────────────────────────────────────────────────

struct DataManager::Impl {
    ubcbaja::Data masterData;
};

// ─── Singleton ────────────────────────────────────────────────────────────────

DataManager::DataManager() : impl_(new Impl()) {}

DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

ubcbaja::Data DataManager::getLatestData() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return impl_->masterData;
}

// ─── Vehicle Telemetry ────────────────────────────────────────────────────────

void DataManager::setTach(uint32_t rpm) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    impl_->masterData.set_tach(rpm);
}

void DataManager::setSpeed(uint32_t speed) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    impl_->masterData.set_speedo(speed);
}

void DataManager::setTemp(uint32_t temp) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    impl_->masterData.set_temp(temp);
}

void DataManager::setFuel(uint32_t fuel) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    impl_->masterData.set_fuel(fuel);
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
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (auto* ecu = getEcu(impl_->masterData, pos))
        ecu->set_travel(travel);
}

void DataManager::setEcuStrain(EcuPosition pos, float strain_l, float strain_r) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (auto* ecu = getEcu(impl_->masterData, pos)) {
        if (strain_l != 0.0f) ecu->set_strain_l(strain_l);
        if (strain_r != 0.0f) ecu->set_strain_r(strain_r);
    }
}

void DataManager::setEcuAccel(EcuPosition pos, float x, float y, float z) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (auto* ecu = getEcu(impl_->masterData, pos)) {
        auto* accel = ecu->mutable_accel();
        accel->set_x(x);
        accel->set_y(y);
        accel->set_z(z);
    }
}

void DataManager::setEcuGyro(EcuPosition pos, float x, float y, float z) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (auto* ecu = getEcu(impl_->masterData, pos)) {
        auto* gyro = ecu->mutable_gyro();
        gyro->set_x(x);
        gyro->set_y(y);
        gyro->set_z(z);
    }
}

// ─── GPS ──────────────────────────────────────────────────────────────────────

void DataManager::setGps(float latitude, float longitude, float speed, bool has_fix) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    auto* gps = impl_->masterData.mutable_location();
    gps->set_latitude(latitude);
    gps->set_longitude(longitude);
    gps->set_gps_speed(speed);
    gps->set_has_fix(has_fix);
}