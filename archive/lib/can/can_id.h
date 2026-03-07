/**
 * @file can_id.h
 * @brief A collection of constant id values for a given device. All the values
 *  in this file are 32 bit values that represent CAN ids for either 11-bit
 *  standard CAN frame ids, or 29-bit extended CAN frame ids.
 */

#ifndef CAN_ID_H
#define CAN_ID_H 

/**
 * @brief The id for the speedometer sensor.
 */
static const int speedometer_id =   0x201;

/**
 * @brief The id for the thermometer sensor.
 */
static const int thermometer_id =   0x203;

/**
 * @brief The id for the tachometer sensor.
 */
static const int tachometer_id =    0x200;

/**
 * @brief The id for the fuel sensor.
 */
static const int fuel_sensor_id =   0x202;

/*
 * @brief The ids for e-suspension signals on all 4 ECUs 
 */
static const int fl_imu_accel_id =  0x100;
static const int fl_imu_gyro_id =   0x101;
static const int fl_suspension =    0x102;
static const int fl_strain_l_id =   0x103;
static const int fl_strain_r_id =   0x104;

static const int fr_imu_accel_id =  0x110;
static const int fr_imu_gyro_id =   0x111;
static const int fr_suspension =    0x112;
static const int fr_strain_l_id =   0x113;
static const int fr_strain_r_id =   0x114;

static const int rl_imu_accel_id =  0x120;
static const int rl_imu_gyro_id =   0x121;
static const int rl_suspension =    0x122;
static const int rl_strain_l_id =   0x123;
static const int rl_strain_r_id =   0x124;

static const int rr_imu_accel_id =  0x130;
static const int rr_imu_gyro_id =   0x131;
static const int rr_suspension =    0x132;
static const int rr_strain_l_id =   0x133;
static const int rr_strain_r_id =   0x134;

/*
 * @brief The PI signal ids
 */
static const int pi_imu_accel_id =  0x180;
static const int pi_imu_gyro_id =   0x181;


#endif // CAN_ID_H