#ifndef CAN_MESSAGES_H
#define CAN_MESSAGES_H

#include <stdint.h>

// -----------------------------------------------------------------------------
// 1. CAN ID Definitions
// Grouped by ECU, with unique IDs for each of the 4 ESUS units.
// -----------------------------------------------------------------------------

// -- ESUS (Electronic Suspension Unit) - High Priority Sensor Data (0x100 - 0x1FF) --
// FL: Front-Left (0x100 - 0x10F)
#define CAN_ID_ESUS_FL_IMU_ACCEL       0x100
#define CAN_ID_ESUS_FL_IMU_GYRO        0x101
#define CAN_ID_ESUS_FL_SUSPENSION      0x102
#define CAN_ID_ESUS_FL_STRAIN_L        0x103
#define CAN_ID_ESUS_FL_STRAIN_R        0x104

// FR: Front-Right (0x110 - 0x11F)
#define CAN_ID_ESUS_FR_IMU_ACCEL       0x110
#define CAN_ID_ESUS_FR_IMU_GYRO        0x111
#define CAN_ID_ESUS_FR_SUSPENSION      0x112
#define CAN_ID_ESUS_FR_STRAIN_L        0x113
#define CAN_ID_ESUS_FR_STRAIN_R        0x114

// RL: Rear-Left (0x120 - 0x12F)
#define CAN_ID_ESUS_RL_IMU_ACCEL       0x120
#define CAN_ID_ESUS_RL_IMU_GYRO        0x121
#define CAN_ID_ESUS_RL_SUSPENSION      0x122
#define CAN_ID_ESUS_RL_STRAIN_L        0x123
#define CAN_ID_ESUS_RL_STRAIN_R        0x124

// RR: Rear-Right (0x130 - 0x13F)
#define CAN_ID_ESUS_RR_IMU_ACCEL       0x130
#define CAN_ID_ESUS_RR_IMU_GYRO        0x131
#define CAN_ID_ESUS_RR_SUSPENSION      0x132
#define CAN_ID_ESUS_RR_STRAIN_L        0x133
#define CAN_ID_ESUS_RR_STRAIN_R        0x134

// -- PI (Platform Interface / Central IMU) (0x180 - 0x18F) --
#define CAN_ID_PI_IMU_ACCEL            0x180
#define CAN_ID_PI_IMU_GYRO             0x181

// -- Rear ECU (Engine/Powertrain Data) (0x200 - 0x20F) --
//#define CAN_ID_REAR_RPM                0x100 // Tachometer
#define CAN_ID_REAR_RPM                0x200 // Tachometer
//#define CAN_ID_REAR_SPEED              0x101 // Speed
#define CAN_ID_REAR_SPEED              0x201 // Speed
#define CAN_ID_REAR_FUEL               0x201 // Fuel
#define CAN_ID_REAR_TEMPERATURE        0x201 // Temperature

// -----------------------------------------------------------------------------
// 2. Data Structure Definitions (Payloads)
// The payload structures can be RE-USED, as the ID defines the source.
// -----------------------------------------------------------------------------

// ESUS Acceleration Data (IMU) - USED by all 4 ESUS units and the PI
typedef struct {
    int16_t accel_x_g_x100; // Acceleration X, scaled by 100 (e.g., 150 -> 1.50g)
    int16_t accel_y_g_x100; // Acceleration Y
    int16_t accel_z_g_x100; // Acceleration Z
} CanMsg_Accel_t; // Simplified name as it's used broadly

// ESUS Angular Velocity Data (IMU) - USED by all 4 ESUS units and the PI
typedef struct {
    int16_t gyro_x_dps_x10; // Angular Velocity X, scaled by 10 (e.g., 345 -> 34.5 deg/s)
    int16_t gyro_y_dps_x10; // Angular Velocity Y
    int16_t gyro_z_dps_x10; // Angular Velocity Z
} CanMsg_Gyro_t; // Simplified name as it's used broadly

// ESUS Suspension Displacement - USED by all 4 ESUS units
typedef struct {
    uint16_t suspension_mm; // Suspension position in mm
    uint16_t suspension_velocity_mmps_x10;
} CanMsg_Suspension_t; // Simplified name: No need for L/R in payload

// ESUS Strain Gauge Data - USED by all 4 ESUS units
typedef struct {
    int16_t strain_milli_N_x10; // Strain/Force scaled by 10 (e.g., 500 -> 50.0 mN)
} CanMsg_Strain_t; // Simplified name

// Rear Powertrain Block A (RPM, Speed)
typedef struct {
    uint16_t tachometer_rpm;    // Engine RPM (1:1 scale)
    uint16_t speed_kmh_x100;    // Vehicle Speed in km/h, scaled by 100
} CanMsg_Rear_PowertrainA_t;

// Rear Powertrain Block B (Fuel, Temperature)
typedef struct {
    uint8_t fuel_level_pct;     // Fuel level in percent (0-100)
    int16_t engine_temp_C_x10;  // Engine Temperature in Celsius, scaled by 10
} CanMsg_Rear_PowertrainB_t;

// -----------------------------------------------------------------------------
// 3. Helper Macros for Scaling (Remain the same)
// -----------------------------------------------------------------------------

// Example: To convert 1.5g to CAN unit (150)
#define ACCEL_TO_CAN(g)         ((int16_t)((g) * 100.0f))
// Example: To convert CAN unit (150) back to engineering unit (1.5g)
#define CAN_TO_ACCEL(can_val)   (((float)(can_val)) / 100.0f)

#endif // CAN_MESSAGES_H