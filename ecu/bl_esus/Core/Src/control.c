#include "control.h"
#include <math.h> 
#include <string.h>

// =============================================================================
// GLOBAL STATE DATA
// =============================================================================

// State container for all local sensor readings
static esus_local_data_t g_esus_data = {0};

// Command container for actuator control (Output of Control, Input of Output)
static esus_actuator_cmd_t g_actuator_command = {0};

static uint16_t s_prev_position_mm = 0;

// =============================================================================
// POTENTIOMETER CALIBRATION AND CONVERSION
// Using constants from the previous Arduino code (distances in cm, voltages in V).
// =============================================================================

#define NUM_SAMPLES 10         // Number of readings for averaging
#define ADC_MAX_VALUE 1023     // Maximum raw value (10-bit ADC)
#define V_REF 5.0f             // ADC Reference Voltage in Volts (5V)

// Calibration Points (Distance in cm, Voltage in V)
#define X_EXTEND 72.39f        // Maximum physical length (at V_EXTEND)
#define X_COMP 47.625f         // Minimum physical length (at V_COMP)
#define V_EXTEND 0.0f          // Voltage at X_EXTEND (0V)
#define V_COMP 5.0f            // Voltage at V_COMP (5V)

/**
 * @brief Converts voltage to physical distance using linear interpolation.
 * @param volt Voltage read from the potentiometer.
 * @return Distance in centimeters (cm).
 */
static float V_to_X(float volt) {
    float x_cm;
    // Linear Interpolation: X = X_EXTEND + V * ( (X_EXTEND - X_COMP) / (V_EXTEND - V_COMP) )
    
    // Safety check against division by zero
    if (V_EXTEND == V_COMP) return 0.0f;

    x_cm = X_EXTEND + volt * ((X_EXTEND - X_COMP) / (V_EXTEND - V_COMP));
    
    return x_cm;
}


// =============================================================================
// SIMULATED HARDWARE INTERFACE FUNCTIONS
// NOTE: These must be replaced with actual STM32 HAL/LL calls interacting with
// the relevant peripherals (ADC, SPI, I2C, CAN, PWM timers).
// =============================================================================

// Placeholder for reading the IMU via SPI/I2C
static void HW_Read_IMU_Raw(int16_t *accel_x, int16_t *accel_y, int16_t *accel_z, 
                            int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z) {
    // --- Simulation placeholder for IMU ---
    *accel_x = 100; *accel_y = 50; *accel_z = 9810; 
    *gyro_x = 100; *gyro_y = -50; *gyro_z = 20;    
}

// Placeholder for reading the Linear Potentiometer via ADC
static uint16_t HW_Read_Potentiometer_Raw(void) {
    // return HAL_ADC_GetValue(&hadc_potentiometer);
    // --- Simulation placeholder: returns raw ADC count (0-1023) ---
    return 512; 
}

// Placeholder for reading the Strain Gauge via ADC
static uint16_t HW_Read_Strain_Gauge_Raw(void) {
    // return HAL_ADC_GetValue(&hadc_strain_gauge);
    // --- Simulation placeholder ---
    return 1200; 
}

// Placeholder for driving the physical actuator (PWM/DAC/Motor Driver)
static void HW_Set_Actuator_Current_mA(int16_t current_mA) {
    // PWM duty cyle update
}


/**
 * @brief Helper function to transmit a CAN message using the provided hardware configuration.
 * @param hcan Pointer to the CAN peripheral handle.
 * @param TxHeader Pointer to the CAN transmit header structure.
 * @param aData Pointer to the 8-byte data payload buffer.
 * @param TxMailbox Pointer to the TxMailbox variable.
 * @param id The standard CAN ID to transmit.
 * @param message Pointer to the message data structure.
 */
void CAN_Transmit_Message(
    CAN_HandleTypeDef *hcan, 
    CAN_TxHeaderTypeDef *TxHeader, 
    uint8_t aData[], 
    uint32_t *TxMailbox, 
    uint32_t id, 
	const void* newData)
{
    // 1. Configure the ID and DLC in the header
    TxHeader->StdId = id;

    // 2. Copy the message data into the transmission buffer
    memcpy(aData, newData, sizeof(aData));

    // 3. Transmit the message
    if (HAL_CAN_AddTxMessage(hcan, TxHeader, aData, TxMailbox) != HAL_OK) {
        // handle error
    }
}

// // Placeholder for CAN bus transmission
// static void CAN_Transmit_Message(uint32_t id, const uint8_t* data) {
//     // HAL_CAN_AddTxMessage(hcan, &TxHeader, (uint8_t*)data, &TxMailbox);
// 	can_header.StdId = id;
// 	memcpy(&can_data, &data, sizeof(can_data));
// 	HAL_CAN_AddTxMessage(&hcan, &can_header, can_data, &can_mailbox);
// }


// =============================================================================
// PHASE 1: ESUS_Sense (Input)
// =============================================================================

/**
 * @brief Phase 1: Sense - Reads all local sensors, filters data, and converts to engineering units.
 */
void ESUS_Sense(void) {
    // raw_[0] = <x>, raw_[1] = <y>, raw_[2] = <z>
    int16_t raw_accel[3], raw_gyro[3];
    uint16_t raw_strain;
    float pot_x_cm_sum = 0.0f; 
    
    // 1. IMU and Strain Gauge
    HW_Read_IMU_Raw(&raw_accel[0], &raw_accel[1], &raw_accel[2], 
                    &raw_gyro[0], &raw_gyro[1], &raw_gyro[2]);
    raw_strain = HW_Read_Strain_Gauge_Raw();

    // 2. Linear Potentiometer Reading (Averaged over NUM_SAMPLES)
    for(int i = 0; i < NUM_SAMPLES; i++){
        uint16_t raw_pot = HW_Read_Potentiometer_Raw();
        
        // Convert raw ADC value to voltage
        float voltage = (float)raw_pot * (V_REF / ADC_MAX_VALUE);
        
        // Convert voltage to distance in centimeters
        pot_x_cm_sum += V_to_X(voltage);;
    }
    float avg_x_cm = pot_x_cm_sum / NUM_SAMPLES;
    
    // --- Store Results in Global State Structure ---
    
    // IMU Acceleration Scaling (g * 100)
    g_esus_data.accel_x_g_x100 = raw_accel[0];
    g_esus_data.accel_y_g_x100 = raw_accel[1];
    g_esus_data.accel_z_g_x100 = raw_accel[2];
    
    // IMU Gyro Scaling (deg/s * 10)
    g_esus_data.gyro_x_dps_x10 = raw_gyro[0];
    g_esus_data.gyro_y_dps_x10 = raw_gyro[1];
    g_esus_data.gyro_z_dps_x10 = raw_gyro[2];

    // Potentiometer Position (Convert cm to mm and round to nearest integer)
    uint16_t current_position_mm = (uint16_t)roundf(avg_x_cm * 10.0f);
    g_esus_data.suspension_position_mm = current_position_mm;

    // Calculate Suspension Velocity
    float velocity_mm_s = ((float)current_position_mm - s_prev_position_mm) / CONTROL_PERIOD_S;
    g_esus_data.suspension_velocity_mmps_x10 = (int16_t)roundf(velocity_mm_s * 10.0f);

    s_prev_position_mm = current_position_mm;

    // Strain Gauge Scaling
    g_esus_data.strain_force_mN_x10 = (int16_t)((raw_strain * 5) / 10); 
    
    g_esus_data.sensor_data_fresh = true;
}

// =============================================================================
// PHASE 2: ESUS_Control (Logic)
// =============================================================================

/**
 * @brief Phase 2: Control - Executes the core suspension control algorithm and calculates actuator command.
 */
void ESUS_Control(void) {
    if (!g_esus_data.sensor_data_fresh) {
        return; 
    }
    
    // --- Simplified Control Algorithm Placeholder (PID Example) ---
    
    // 1. Calculate Error (Target ride height is 500mm)
    int16_t position_error = 500 - (int16_t)g_esus_data.suspension_position_mm;
    
    // 2. Proportional Term (Kp = 10)
    int32_t proportional_cmd = position_error * 10;
    
    // 3. Damping Term (Placeholder: resistance based on angular velocity)
    int32_t damping_term = g_esus_data.gyro_z_dps_x10 * 2;

    // 4. Calculate Total Command (Target current in mA)
    int32_t total_command_mA = (proportional_cmd - damping_term) / 10;
    
    // 5. Apply Saturation/Clamping 
    if (total_command_mA > 1000) total_command_mA = 1000;
    if (total_command_mA < -1000) total_command_mA = -1000;

    // 6. Update Actuator Command Structure
    g_actuator_command.actuator_current_mA = (int16_t)total_command_mA;
    g_esus_data.sensor_data_fresh = false; // Data is now consumed
}


// =============================================================================
// PHASE 3: ESUS_Publish (Actuate and Communicate)
// =============================================================================

/**
 * @brief Phase 3: Publish - Drives the physical actuator and publishes system data onto the CAN bus.
 */
void ESUS_Publish(
    CAN_HandleTypeDef *hcan, 
    CAN_TxHeaderTypeDef *TxHeader, 
    uint8_t aData[], 
    uint32_t *TxMailbox
) {
    // 1. Actuation (Output)
    HW_Set_Actuator_Current_mA(g_actuator_command.actuator_current_mA);

    
    // A. Acceleration Message
    CanMsg_Accel_t accel_msg = {
        .accel_x_g_x100 = g_esus_data.accel_x_g_x100,
        .accel_y_g_x100 = g_esus_data.accel_y_g_x100,
        .accel_z_g_x100 = g_esus_data.accel_z_g_x100,
    };
    CAN_Transmit_Message(hcan, TxHeader, aData, TxMailbox, CAN_ID_ESUS_RL_IMU_ACCEL, (const uint8_t*)&accel_msg);
    
    // B. Gyro Message
    CanMsg_Gyro_t gyro_msg = {
        .gyro_x_dps_x10 = g_esus_data.gyro_x_dps_x10,
        .gyro_y_dps_x10 = g_esus_data.gyro_y_dps_x10,
        .gyro_z_dps_x10 = g_esus_data.gyro_z_dps_x10,
    };
    CAN_Transmit_Message(hcan, TxHeader, aData, TxMailbox, CAN_ID_ESUS_RL_IMU_GYRO, (const uint8_t*)&gyro_msg);

    // C. Suspension Position Message
    CanMsg_Suspension_t susp_msg = {
        .suspension_mm = g_esus_data.suspension_position_mm,
        .suspension_velocity_mmps_x10 = g_esus_data.suspension_velocity_mmps_x10
    };
    CAN_Transmit_Message(hcan, TxHeader, aData, TxMailbox, CAN_ID_ESUS_RL_SUSPENSION, (const uint8_t*)&susp_msg);

    // D. Strain Gauge Message
    CanMsg_Strain_t strain_msg = {
        .strain_milli_N_x10 = g_esus_data.strain_force_mN_x10
    };
    CAN_Transmit_Message(hcan, TxHeader, aData, TxMailbox, CAN_ID_ESUS_RL_STRAIN_L, (const uint8_t*)&strain_msg);
}

