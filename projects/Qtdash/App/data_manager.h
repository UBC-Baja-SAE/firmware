#include <QObject>
#include <stdint.h>

class DataManager : public QObject {
    Q_OBJECT
    // Expose these properties to QML
    Q_PROPERTY(float speed READ speed NOTIFY speedChanged)
    Q_PROPERTY(float tach READ tach NOTIFY tachChanged)

public:
    enum EcuPosition { FRONT_LEFT, FRONT_RIGHT, REAR_LEFT, REAR_RIGHT };

    static DataManager& getInstance() {
        static DataManager instance;
        return instance;
    }

    float speed() const { return m_speed; }
    float tach() const { return m_tach; }

    // Called by can_bridge.cpp from the background thread
    void setSpeed(uint32_t val) {
        if (m_speed == val) return;
        m_speed = val;
        emit speedChanged(); // Notifies QML to update the gauge
    }

    void setTach(uint32_t val) {
        if (m_tach == val) return;
        m_tach = val;
        emit tachChanged(); // Notifies QML to update the gauge
    }

    // Stubs for the other ECU functions in can_bridge.cpp so it compiles
    void setTemp(uint32_t) {}
    void setFuel(uint32_t) {}
    void setEcuTravel(EcuPosition, float) {}
    void setEcuStrain(EcuPosition, float, float) {}
    void setEcuAccel(EcuPosition, float, float, float) {}
    void setEcuGyro(EcuPosition, float, float, float) {}

signals:
    void speedChanged();
    void tachChanged();

private:
    DataManager() = default; // Private constructor for Singleton
    float m_speed = 0;
    float m_tach = 0;
};
