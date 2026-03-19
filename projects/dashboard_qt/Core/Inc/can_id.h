#ifndef DASHBOARD_QT_CAN__ID_H
#define DASHBOARD_QT_CAN__ID_H
#include <stdint.h>

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

// -- Rear ECU (Engine/Powertrain Data) (0x200 - 0x20F) --
#define CAN_ID_REAR_RPM                0x200 // Tachometer
#define CAN_ID_REAR_SPEED              0x201 // Speed
#define CAN_ID_REAR_FUEL               0x201 // Fuel
#define CAN_ID_REAR_TEMPERATURE        0x201 // Temperature

#endif //DASHBOARD_QT_CAN__ID_H