#ifndef DASHBOARD_QT_DATA_MANAGER_H
#define DASHBOARD_QT_DATA_MANAGER_H

#include <mutex>
#include "Core/Src/data.pb.h"

class DataManager {
public:
    static DataManager& getInstance();

    DataManager(DataManager const&) = delete;
    void operator=(DataManager const&) = delete;

    test::Data getLatestData();

    // Dashboard sensors
    void setTach(uint32_t rpm);
    void setSpeed(uint32_t speed);
    void setTemp(uint32_t temp);
    void setFuel(uint32_t fuel);

    enum EcuPosition {
        FRONT_LEFT  = 0,
        FRONT_RIGHT = 1,
        REAR_LEFT   = 2,
        REAR_RIGHT  = 3
    };

    // ECU sensors
    void setEcuTravel(EcuPosition pos, float travel);
    void setEcuStrain(EcuPosition pos, float strain_l, float strain_r);
    void setEcuAccel(EcuPosition pos, float x, float y, float z);
    void setEcuGyro(EcuPosition pos, float x, float y, float z);

    // GPS
    void setGps(float latitude, float longitude, float speed, bool has_fix);

private:
    DataManager() = default;

    static test::ECU* getEcu(test::Data& data, EcuPosition pos);

    test::Data  masterData_;
    std::mutex  dataMutex_;
};

#endif //DASHBOARD_QT_DATA_MANAGER_H