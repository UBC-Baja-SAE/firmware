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

void DataManager::setEcuTravel(EcuPosition pos, float travel) {
    std::lock_guard<std::mutex> lock(dataMutex_);

    switch (pos) {
    case FRONT_LEFT:  masterData_.mutable_ecu_fl()->set_travel(travel); break;
    case FRONT_RIGHT: masterData_.mutable_ecu_fr()->set_travel(travel); break;
    case REAR_LEFT:   masterData_.mutable_ecu_rl()->set_travel(travel); break;
    case REAR_RIGHT:  masterData_.mutable_ecu_rr()->set_travel(travel); break;
    }
}