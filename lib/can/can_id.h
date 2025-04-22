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
static const int speedometer_id =   0x00000001;

/**
 * @brief The id for the thermometer sensor.
 */
static const int thermometer_id =   0x00000002;

/**
 * @brief The id for the tachometer sensor.
 */
static const int tachometer_id =    0x00000003;

/**
 * @brief The id for the fuel sensor.
 */
static const int fuel_sensor_id =   0x00000004;

#endif // CAN_ID_H