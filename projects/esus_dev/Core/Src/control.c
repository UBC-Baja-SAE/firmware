#include "control.h"
#include <string.h>

extern ADC_HandleTypeDef hadc1;
extern FDCAN_HandleTypeDef hfdcan1;
extern I2C_HandleTypeDef hi2c1;

uint8_t pot_tx_data[2];

#define GYRO_SENSITIVITY_2000DPS 16.4f
#define ACCEL_SENSITIVITY_16G 2048.0f

static void CAN_Transmit(uint32_t id, uint8_t *data, uint32_t len) {
    FDCAN_TxHeaderTypeDef tx_header = {
        .Identifier         = id,
        .IdType             = FDCAN_STANDARD_ID,
        .TxFrameType        = FDCAN_DATA_FRAME,
        .DataLength         = len,
        .ErrorStateIndicator= FDCAN_ESI_ACTIVE,
        .BitRateSwitch      = FDCAN_BRS_OFF,
        .FDFormat           = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker      = 0
    };

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, data) != HAL_OK) {
        Error_Handler();
    }
}

float VoltageToPosition(float voltage) {
    return X_EXTEND_CM +
           (voltage * (X_EXTEND_CM - X_COMP_CM) / (V_EXTEND - V_COMP));
}

// void SendGyroOnCan(uint32_t can_id) {
//     uint8_t raw_data[6] = {0};
//
//     if (!g_imu_initialized) return;
//
//     if (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
//         HAL_I2C_Master_Abort_IT(&hi2c1, ICM_ADDR);
//         HAL_Delay(5);
//         if (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
//             I2C_BusRecovery();
//             IMU_Init();
//             return;
//         }
//     }
//
//     g_debug_i2c_status = HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, GYRO_DATA_START,
//                                           I2C_MEMADD_SIZE_8BIT, raw_data, 6, 50);
//
//     if (g_debug_i2c_status == HAL_OK) {
//         // Convert to signed 16-bit for CLion Live Watch
//         int16_t x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
//         int16_t y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
//         int16_t z = (int16_t)((raw_data[4] << 8) | raw_data[5]);
//
//         live_gyro_x = (float)x / GYRO_SENSITIVITY_2000DPS;
//         live_gyro_y = (float)y / GYRO_SENSITIVITY_2000DPS;
//         live_gyro_z = (float)z / GYRO_SENSITIVITY_2000DPS;
//
//         CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);
//     } else {
//         if (I2C_BusRecovery() == HAL_OK) IMU_Init();
//     }
// }
//
// void SendAccelOnCan(uint32_t can_id) {
//     uint8_t raw_data[6] = {0};
//
//     if (!a_imu_initialized) return;
//
//     if (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
//         HAL_I2C_Master_Abort_IT(&hi2c1, ICM_ADDR);
//         HAL_Delay(5);
//         if (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
//             I2C_BusRecovery();
//             IMU_Init();
//             return;
//         }
//     }
//
//     a_debug_i2c_status = HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, ACCEL_DATA_START,
//                                           I2C_MEMADD_SIZE_8BIT, raw_data, 6, 50);
//
//     if (a_debug_i2c_status == HAL_OK) {
//         // Convert to signed 16-bit for CLion Live Watch
//         int16_t x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
//         int16_t y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
//         int16_t z = (int16_t)((raw_data[4] << 8) | raw_data[5]);
//
//         live_accel_x = (float)x / ACCEL_SENSITIVITY_16G;
//         live_accel_y = (float)y / ACCEL_SENSITIVITY_16G;
//         live_accel_z = (float)z / ACCEL_SENSITIVITY_16G;
//
//         CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);
//     } else {
//         if (I2C_BusRecovery() == HAL_OK) IMU_Init();
//     }
// }

void SendPotOnCan(uint32_t can_id) {
    const int samples = 10;
    float posSum = 0;
    ADC_ChannelConfTypeDef sConfig = {0};

    // Configure ADC1 to the Potentiometer channel
    sConfig.Channel = ADC_CHANNEL_5;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    for (int i = 0; i < samples; i++) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            uint32_t adcRaw = HAL_ADC_GetValue(&hadc1);
            float voltage = ((float)adcRaw * 1.8f) / 65535.0f;
            float position = VoltageToPosition(voltage);
            posSum += position;
        }
        HAL_ADC_Stop(&hadc1);
    }

    float posAvg = posSum / samples;
    uint16_t posCan = (uint16_t)(posAvg * 100.0f);
    pot_tx_data[0] = (posCan >> 8) & 0xFF;
    pot_tx_data[1] = posCan & 0xFF;

    CAN_Transmit(can_id, pot_tx_data, FDCAN_DLC_BYTES_2);
}
