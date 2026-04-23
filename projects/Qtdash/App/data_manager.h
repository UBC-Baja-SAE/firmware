#pragma once

#include <mutex>
#include <cstdint>

// Forward-declare the generated protobuf type so that translation units
// that only need setters do not have to pull in the heavy generated header.
namespace ubcbaja { class Data; }

class DataManager {
public:
    enum EcuPosition { FRONT_LEFT, FRONT_RIGHT, REAR_LEFT, REAR_RIGHT };

    static DataManager& getInstance();

    // Returns a snapshot of the current telemetry frame (thread-safe copy).
    ubcbaja::Data getLatestData();

    // ── Powertrain ────────────────────────────────────────────────────────────
    void setTach (uint32_t rpm);
    void setSpeed(uint32_t speed);
    void setTemp (uint32_t temp);
    void setFuel (uint32_t fuel);

    // ── Per-corner ECU ────────────────────────────────────────────────────────
    void setEcuTravel(EcuPosition pos, float travel);
    void setEcuStrain(EcuPosition pos, float strain_l, float strain_r);
    void setEcuAccel (EcuPosition pos, float x, float y, float z);
    void setEcuGyro  (EcuPosition pos, float x, float y, float z);

    // ── GPS ───────────────────────────────────────────────────────────────────
    void setGps(float latitude, float longitude, float speed, bool has_fix);

private:
    DataManager();
    ~DataManager() = default;
    DataManager(const DataManager&)            = delete;
    DataManager& operator=(const DataManager&) = delete;

    // The actual protobuf object is held by value here.
    // We use a forward-declared pointer to avoid including the generated header
    // in every translation unit — include data_manager_impl.h if you need it.
    struct Impl;
    Impl* impl_;

    std::mutex dataMutex_;
};