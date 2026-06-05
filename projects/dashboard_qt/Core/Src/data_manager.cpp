#include "../Inc/data_manager.h"

DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

ubcbaja::Data DataManager::getLatestData() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return masterData_;
}

// ─── Vehicle Telemetry ────────────────────────────────────────────────────────

void DataManager::setTach(uint32_t rpm) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    masterData_.set_tach(rpm);
}

void DataManager::setSpeed(uint32_t speed) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    masterData_.set_speedo(speed);
}

void DataManager::setTemp(uint32_t temp) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    masterData_.set_temp(temp);
}

void DataManager::setFuel(uint32_t fuel) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    masterData_.set_fuel(fuel);
}

// ─── ECU Helpers ──────────────────────────────────────────────────────────────

ubcbaja::ECU* DataManager::getEcu(ubcbaja::Data& data, EcuPosition pos) {
    switch (pos) {
    case FRONT_LEFT:  return data.mutable_ecu_fl();
    case FRONT_RIGHT: return data.mutable_ecu_fr();
    case REAR_LEFT:   return data.mutable_ecu_rl();
    case REAR_RIGHT:  return data.mutable_ecu_rr();
    default:          return nullptr;
    }
}

// ─── ECU Setters ──────────────────────────────────────────────────────────────

void DataManager::setEcuTravel(EcuPosition pos, float travel) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (auto* ecu = getEcu(masterData_, pos))
        ecu->set_travel(travel);
}

void DataManager::setEcuStrain(EcuPosition pos, float strain_l, float strain_r) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (auto* ecu = getEcu(masterData_, pos)) {
        if (strain_l != 0.0f) ecu->set_strain_l(strain_l);
        if (strain_r != 0.0f) ecu->set_strain_r(strain_r);
    }
}

void DataManager::setEcuAccel(EcuPosition pos, float x, float y, float z) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (auto* ecu = getEcu(masterData_, pos)) {
        auto* accel = ecu->mutable_accel();
        accel->set_x(x);
        accel->set_y(y);
        accel->set_z(z);
    }
}

void DataManager::setEcuGyro(EcuPosition pos, float x, float y, float z) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (auto* ecu = getEcu(masterData_, pos)) {
        auto* gyro = ecu->mutable_gyro();
        gyro->set_x(x);
        gyro->set_y(y);
        gyro->set_z(z);
    }
}

// ─── GPS ──────────────────────────────────────────────────────────────────────

void DataManager::setGps(float latitude, float longitude, float speed, bool has_fix) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    auto* gps = masterData_.mutable_location();
    gps->set_latitude(latitude);
    gps->set_longitude(longitude);
    gps->set_gps_speed(speed);
    gps->set_has_fix(has_fix);
}