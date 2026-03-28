#include "../Inc/data_manager.h"

DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

test::Data DataManager::getLatestData() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return masterData_;
}

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

void DataManager::setEcuTravel(EcuPosition pos, float travel) {
    std::lock_guard<std::mutex> lock(dataMutex_);

    switch (pos) {
    case FRONT_LEFT:  masterData_.mutable_ecu_fl()->set_travel(travel); break;
    case FRONT_RIGHT: masterData_.mutable_ecu_fr()->set_travel(travel); break;
    case REAR_LEFT:   masterData_.mutable_ecu_rl()->set_travel(travel); break;
    case REAR_RIGHT:  masterData_.mutable_ecu_rr()->set_travel(travel); break;
    }
}

void DataManager::setEcuStrain(EcuPosition pos, float strain_l, float strain_r) {
    std::lock_guard<std::mutex> lock(dataMutex_);

    switch (pos) {
    case FRONT_LEFT:
        if (strain_l != 0.0f) masterData_.mutable_ecu_fl()->set_strain_l(strain_l);
        if (strain_r != 0.0f) masterData_.mutable_ecu_fl()->set_strain_r(strain_r);
        break;
    case FRONT_RIGHT:
        if (strain_l != 0.0f) masterData_.mutable_ecu_fr()->set_strain_l(strain_l);
        if (strain_r != 0.0f) masterData_.mutable_ecu_fr()->set_strain_r(strain_r);
        break;
    case REAR_LEFT:
        if (strain_l != 0.0f) masterData_.mutable_ecu_rl()->set_strain_l(strain_l);
        if (strain_r != 0.0f) masterData_.mutable_ecu_rl()->set_strain_r(strain_r);
        break;
    case REAR_RIGHT:
        if (strain_l != 0.0f) masterData_.mutable_ecu_rr()->set_strain_l(strain_l);
        if (strain_r != 0.0f) masterData_.mutable_ecu_rr()->set_strain_r(strain_r);
        break;
    }
}

void DataManager::setEcuAccel(EcuPosition pos, float x, float y, float z) {
    std::lock_guard<std::mutex> lock(dataMutex_);

    // Note: Protobuf uses Quaternion but accel is 3-axis. Store in orientation for now
    // You may want to update the protobuf schema or add separate accel fields
    switch (pos) {
    case FRONT_LEFT:
        masterData_.mutable_ecu_fl()->mutable_orientation()->set_x(x);
        masterData_.mutable_ecu_fl()->mutable_orientation()->set_y(y);
        masterData_.mutable_ecu_fl()->mutable_orientation()->set_z(z);
        break;
    case FRONT_RIGHT:
        masterData_.mutable_ecu_fr()->mutable_orientation()->set_x(x);
        masterData_.mutable_ecu_fr()->mutable_orientation()->set_y(y);
        masterData_.mutable_ecu_fr()->mutable_orientation()->set_z(z);
        break;
    case REAR_LEFT:
        masterData_.mutable_ecu_rl()->mutable_orientation()->set_x(x);
        masterData_.mutable_ecu_rl()->mutable_orientation()->set_y(y);
        masterData_.mutable_ecu_rl()->mutable_orientation()->set_z(z);
        break;
    case REAR_RIGHT:
        masterData_.mutable_ecu_rr()->mutable_orientation()->set_x(x);
        masterData_.mutable_ecu_rr()->mutable_orientation()->set_y(y);
        masterData_.mutable_ecu_rr()->mutable_orientation()->set_z(z);
        break;
    }
}

void DataManager::setEcuGyro(EcuPosition pos, float x, float y, float z) {
    std::lock_guard<std::mutex> lock(dataMutex_);

    // Store gyro w component in quaternion.w for now
    switch (pos) {
    case FRONT_LEFT:  masterData_.mutable_ecu_fl()->mutable_orientation()->set_w(x); break;
    case FRONT_RIGHT: masterData_.mutable_ecu_fr()->mutable_orientation()->set_w(x); break;
    case REAR_LEFT:   masterData_.mutable_ecu_rl()->mutable_orientation()->set_w(x); break;
    case REAR_RIGHT:  masterData_.mutable_ecu_rr()->mutable_orientation()->set_w(x); break;
    }
}

void DataManager::setGps(float latitude, float longitude, float speed, bool has_fix) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    masterData_.mutable_location()->set_latitude(latitude);
    masterData_.mutable_location()->set_longitude(longitude);
    masterData_.mutable_location()->set_gps_speed(speed);
    masterData_.mutable_location()->set_has_fix(has_fix);
}