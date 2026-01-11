#ifndef CONTROL_H
#define CONTROL_H

//#ifdef __cplusplus
//extern "C" {
//#endif

#include <stdint.h>
#include <stdbool.h>
#include "can_messages.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_fdcan.h"

#define CONTROL_PERIOD_S  (10.0f / 1000.0f)


// -----------------------------------------------------------------------------
// Local Data Structures
// -----------------------------------------------------------------------------

/**
 * @brief Structure holding all sensor readings for the local ESUS (Sense output).
 * Data is stored in engineering units (or ready-to-use scaled units).
 */
typedef struct {
    // IMU Data (from local sensor)
    int16_t accel_x_g_x100;
    int16_t accel_y_g_x100;
    int16_t accel_z_g_x100;

    int16_t gyro_x_dps_x10;
    int16_t gyro_y_dps_x10;
    int16_t gyro_z_dps_x10;

    // Position, Velocity, and Force Data
    uint16_t suspension_position_mm;
    int16_t suspension_velocity_mmps_x10;
    int16_t strain_force_mN_x10;

    // Status
    bool sensor_data_fresh;
} esus_local_data_t;

/**
 * @brief Structure holding the required physical output command (Control output).
 */
typedef struct {
    int16_t actuator_current_mA; // Target current for the actuator (in milliamps)
    uint16_t max_stroke_limit_mm; // Safety limit for actuator travel
} esus_actuator_cmd_t;


// -----------------------------------------------------------------------------
// Function Declarations
// -----------------------------------------------------------------------------

/**
 * @brief Phase 1: Sense - Reads all local sensors, filters data, and converts to engineering units.
 */
void ESUS_Sense(void);

/**
 * @brief Phase 2: Control - Executes the core suspension control algorithm and calculates actuator command.
 */
void ESUS_Control(void);

/**
 * @brief Phase 3: Publish - Drives the physical actuator and publishes system data onto the CAN bus.
 * @param hcan Pointer to the CAN peripheral handle.
 * @param TxHeader Pointer to the CAN transmit header structure.
 * @param aData Pointer to the 8-byte data payload buffer.
 * @param TxMailbox Pointer to the TxMailbox variable.
 */
void ESUS_Publish(
    FDCAN_HandleTypeDef *hcan,
    FDCAN_TxHeaderTypeDef *TxHeader,
    uint8_t aData[],
    uint32_t *TxMailbox
);

/**
 * @brief Helper function to transmit a CAN message using the provided hardware configuration.
 */
void CAN_Transmit_Message(
    FDCAN_HandleTypeDef *hcan,
    FDCAN_TxHeaderTypeDef *TxHeader,
    uint8_t aData[],
    uint32_t *TxMailbox,
    uint32_t id,
	const void* newData
);

#endif // CONTROL_H
