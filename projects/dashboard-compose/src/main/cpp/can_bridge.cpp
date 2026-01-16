#include <jni.h>
#include <stdint.h>
#include <string.h>
#include "can_bridge.h"
#include "can_processor.h"
#include "can_id.h"

/**
 * @brief Observes the last received CAN message with the given id.
 * @param id    the id of the CAN message to observe.
 * @return the data received from the CAN message. 
 */
double getData(int id) {
    uint64_t data = observed_data[id];

    // Log the Hex to the console so we can see it in the terminal
    if (data != 0) {
        printf("DEBUG: ID 0x%X received 0x%016llx\n", id, (unsigned long long)data);
    }

    if (id >= 0x100 && id <= 0x134) {
        uint16_t val;
        memcpy(&val, &data, 2);
        return (double)val;
    }

    double result;
    memcpy(&result, &data, sizeof(data));
    return result;
}

// --- Standard Dashboard Signals ---
getSpeed
{
    return (jdouble) getData(speedometer_id);
}

getTemperature
{
    return (jdouble) getData(thermometer_id);
}

getRPM
{
    return (jdouble) getData(tachometer_id);
}

getFuel
{
    return (jdouble) getData(fuel_sensor_id);
}

// --- Front Left ECU Signals ---
getFLAccel
{
    return (jdouble) getData(fl_imu_accel_id);
}

getFLGyro
{
    return (jdouble) getData(fl_imu_gyro_id);
}

getFLSuspension
{
    return (jdouble) getData(fl_suspension);
}

getFLStrainL
{
    return (jdouble) getData(fl_strain_l_id);
}

getFLStrainR
{
    return (jdouble) getData(fl_strain_r_id);
}

// --- Front Right ECU Signals ---
getFRAccel
{
    return (jdouble) getData(fr_imu_accel_id);
}

getFRGyro
{
    return (jdouble) getData(fr_imu_gyro_id);
}

getFRSuspension
{
    return (jdouble) getData(fr_suspension);
}

getFRStrainL
{
    return (jdouble) getData(fr_strain_l_id);
}

getFRStrainR
{
    return (jdouble) getData(fr_strain_r_id);
}

// --- Rear Left ECU Signals ---
getRLAccel
{
    return (jdouble) getData(rl_imu_accel_id);
}

getRLGyro
{
    return (jdouble) getData(rl_imu_gyro_id);
}

getRLSuspension
{
    return (jdouble) getData(rl_suspension);
}

getRLStrainL
{
    return (jdouble) getData(rl_strain_l_id);
}

getRLStrainR
{
    return (jdouble) getData(rl_strain_r_id);
}

// --- Rear Right ECU Signals ---
getRRAccel
{
    return (jdouble) getData(rr_imu_accel_id);
}

getRRGyro
{
    return (jdouble) getData(rr_imu_gyro_id);
}

getRRSuspension
{
    return (jdouble) getData(rr_suspension);
}

getRRStrainL
{
    return (jdouble) getData(rr_strain_l_id);
}

getRRStrainR
{
    return (jdouble) getData(rr_strain_r_id);
}

// --- Pi IMU ---
getPiAccel
{
    return (jdouble) getData(pi_imu_accel_id);    
}

getPiGyro
{
    return (jdouble) getData(pi_imu_gyro_id);
}

startProcessor
{
    start();
}