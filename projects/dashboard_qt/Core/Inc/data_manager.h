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

    void setTach(uint32_t rpm);

    enum EcuPosition {
        FRONT_LEFT = 0,
        FRONT_RIGHT = 1,
        REAR_LEFT = 2,
        REAR_RIGHT = 3
    };

    void setEcuTravel(EcuPosition pos, float travel);

private:
    DataManager() {}

    test::Data masterData_;
    std::mutex dataMutex_;
};

#endif //DASHBOARD_QT_DATA_MANAGER_H