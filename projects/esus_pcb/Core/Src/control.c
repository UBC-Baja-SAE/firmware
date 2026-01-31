#include "control.h"

// Hardware Handles
extern ADC_HandleTypeDef hadc1;  // Used for Potentiometer
extern ADC_HandleTypeDef hadc2;  // Used for Strain Gauge (Assumed based on main.c)
extern FDCAN_HandleTypeDef hfdcan1;
extern I2C_HandleTypeDef hi2c1;  // Used for IMU

uint8_t pot_tx_data[2];

// Global variables for CLion Live Watches
volatile float live_gyro_x = 0.0f;
volatile float live_gyro_y = 0.0f;
volatile float live_gyro_z = 0.0f;

volatile float live_accel_x = 0.0f;
volatile float live_accel_y = 0.0f;
volatile float live_accel_z = 0.0f;

volatile float live_pot = 0.0f;

// Sensitivity for ICM-42670-P at default ±2000 dps is 16.4 LSB/dps
#define GYRO_SENSITIVITY_2000DPS 16.4f
#define ACCEL_SENSITIVITY_16G 2048.0f




    static void CAN_Transmit(uint32_t id, uint8_t *data, uint32_t len) {
        FDCAN_TxHeaderTypeDef tx_header = {
            .Identifier = id,
            .IdType             = FDCAN_STANDARD_ID,
            .TxFrameType        = FDCAN_DATA_FRAME,
            .DataLength         = len,
            .ErrorStateIndicator= FDCAN_ESI_ACTIVE,
            .BitRateSwitch      = FDCAN_BRS_OFF,
            .FDFormat           = FDCAN_CLASSIC_CAN,
            .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
            .MessageMarker      = 0
        };

        // CHANGE: Do not call Error_Handler() if the queue is full. Just skip.
        if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, data) != HAL_OK) {
            //Error_Handler();
            return;
        }
}

// Initialization for ICM-42670-P
void IMU_Init(void) {
    // Wake up: Enable both Accel and Gyro in Low Noise mode
    uint8_t pwr_mgmt = 0x0F;
    HAL_I2C_Mem_Write(&hi2c1, ICM_ADDR, PWR_MGMT0, I2C_MEMADD_SIZE_8BIT, &pwr_mgmt, 1, 1000);
}

float VoltageToPosition(float voltage)
{
    return X_EXTEND_CM + (voltage * (X_EXTEND_CM - X_COMP_CM) / (V_EXTEND - V_COMP));
}



void SendPotOnCan(uint32_t can_id)
    {
        const int samples = 10;
        float posSum = 0;

        // DELETE THESE LINES (The ADC is already configured in main.c!)
        ADC_ChannelConfTypeDef sConfig = {0};
     //         Reconfigure ADC1 to the Potentiometer channel (PB1)
     sConfig.Channel = ADC_CHANNEL_5;
     sConfig.Rank = ADC_REGULAR_RANK_1;
     sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5; // Decreased
     sConfig.SingleDiff = ADC_SINGLE_ENDED;
     HAL_ADC_ConfigChannel(&hadc1, &sConfig);

        for (int i = 0; i < samples; i++)
        {
            HAL_ADC_Start(&hadc1);
            if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
            {
                uint32_t adcRaw = HAL_ADC_GetValue(&hadc1);
                float voltage = ((float)adcRaw * 1.8f) / 65535.0f; // Note: H7 is usually 3.3V
                float position = VoltageToPosition(voltage);
                posSum += position;
            }
            HAL_ADC_Stop(&hadc1);
        }

        float posAvg = posSum / samples;

        uint16_t posCan = (uint16_t)(posAvg * 100.0f);
        live_pot = posAvg;
        pot_tx_data[0] = (posCan >> 8) & 0xFF;
        pot_tx_data[1] = posCan & 0xFF;

        CAN_Transmit(can_id, pot_tx_data, FDCAN_DLC_BYTES_2);
    }



// void SendPotOnCan(uint32_t can_id)
// {
//     const int samples = 10;
//     float posSum = 0;
//     ADC_ChannelConfTypeDef sConfig = {0};
//
//     // // Reconfigure ADC1 to the Potentiometer channel (PB1)
//     // sConfig.Channel = ADC_CHANNEL_5;
//     // sConfig.Rank = ADC_REGULAR_RANK_1;
//     // sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5; // Decreased
//     // sConfig.SingleDiff = ADC_SINGLE_ENDED;
//     // HAL_ADC_ConfigChannel(&hadc1, &sConfig);
//
//     for (int i = 0; i < samples; i++)
//     {
//         HAL_ADC_Start(&hadc1);
//         if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
//         {
//             uint32_t adcRaw = HAL_ADC_GetValue(&hadc1);
//             float voltage = ((float)adcRaw * 3.3f) / 65535.0f; // Note: H7 is usually 3.3V
//             float position = VoltageToPosition(voltage);
//             posSum += position;
//         }
//         else
//         {
//             // Add this breakpoint or variable!
//             live_pot = -1.0f; // If you see -1.0 in Live Watch, you know the ADC is timing out!
//         }
//         HAL_ADC_Stop(&hadc1);
//     }
//         float posAvg = posSum / samples;
//
//         uint16_t posCan = (uint16_t)(posAvg * 100.0f);
//         live_pot = (posAvg *100.0f);
//         pot_tx_data[0] = (posCan >> 8) & 0xFF;
//         pot_tx_data[1] = posCan & 0xFF;
//
//         CAN_Transmit(can_id, pot_tx_data, FDCAN_DLC_BYTES_2);
//
// }

void SendAccelOnCan(uint32_t can_id) {
    uint8_t raw_data[6];

    if (HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, ACCEL_DATA_START, I2C_MEMADD_SIZE_8BIT, raw_data, 6, 50) == HAL_OK) {

        // 1. Combine high and low bytes into signed 16-bit integers
        int16_t raw_x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
        int16_t raw_y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
        int16_t raw_z = (int16_t)((raw_data[4] << 8) | raw_data[5]);

        // 2. Convert to Gs (or m/s^2 by multiplying by 9.81)
        live_accel_x = (float)raw_x / ACCEL_SENSITIVITY_16G;
        live_accel_y = (float)raw_y / ACCEL_SENSITIVITY_16G;
        live_accel_z = (float)raw_z / ACCEL_SENSITIVITY_16G;

        // 3. Keep your existing CAN transmission
        CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);
    }
        else {
            HAL_I2C_DeInit(&hi2c1);
            HAL_I2C_Init(&hi2c1);
            return;
        }

}
/*void SendGyroOnCan(uint32_t can_id) {
    uint8_t raw_data[6];
    // Read 6 bytes starting from GYRO_DATA_X1 (0x11)
    if (HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, GYRO_DATA_START, I2C_MEMADD_SIZE_8BIT, raw_data, 6, 100) == HAL_OK) {
        CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);
    }
}*/

void SendGyroOnCan(uint32_t can_id) {
    uint8_t raw_data[6];

    // Read 6 bytes starting from GYRO_DATA_X1 (0x11)
    if (HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, GYRO_DATA_START, I2C_MEMADD_SIZE_8BIT, raw_data, 6, 100) == HAL_OK) {

        // Convert raw bytes to signed 16-bit integers
        int16_t raw_x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
        int16_t raw_y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
        int16_t raw_z = (int16_t)((raw_data[4] << 8) | raw_data[5]);

        // Convert to Degrees Per Second (DPS)
        live_gyro_x = (float)raw_x / GYRO_SENSITIVITY_2000DPS;
        live_gyro_y = (float)raw_y / GYRO_SENSITIVITY_2000DPS;
        live_gyro_z = (float)raw_z / GYRO_SENSITIVITY_2000DPS;

        CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);

    }
    else {
        HAL_I2C_DeInit(&hi2c1);
        HAL_I2C_Init(&hi2c1);
        return;
    }
}




void SendStrainOnCan(uint32_t can_id, uint32_t channel) {
        ADC_ChannelConfTypeDef sConfig = {0};
        uint16_t strain_val = 0;

        // 1. USE THE CORRECT HANDLE (hadc2)
        // 2. Add Error Checking
        sConfig.Channel = channel;
        // ... setup ...

        // If using hadc2:
        if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) return;

        HAL_ADC_Start(&hadc2);
        if (HAL_ADC_PollForConversion(&hadc2, 10) == HAL_OK) {
            strain_val = (uint16_t)HAL_ADC_GetValue(&hadc2);
        }
        HAL_ADC_Stop(&hadc2);

        uint8_t data[2] = { (strain_val >> 8) & 0xFF, strain_val & 0xFF };
        CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_2);
    }

// void SendStrainOnCan(uint32_t can_id, uint32_t channel) {
//     ADC_ChannelConfTypeDef sConfig = {0};
//     uint16_t strain_val = 0;
//
//     // --- FIX STARTS HERE ---
//     // USE hadc2, NOT hadc1
//     sConfig.Channel = channel;
//     sConfig.Rank = ADC_REGULAR_RANK_1;
//     sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5; // Check if ADC2 supports this timing
//     sConfig.SingleDiff = ADC_SINGLE_ENDED;
//
//     // Use hadc2 for all calls here
//     if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
//         // Optional: Add error handling
//     }
//
//     HAL_ADC_Start(&hadc2);
//     if (HAL_ADC_PollForConversion(&hadc2, 10) == HAL_OK) {
//         strain_val = (uint16_t)HAL_ADC_GetValue(&hadc2);
//     }
//     HAL_ADC_Stop(&hadc2);
//     // --- FIX ENDS HERE ---
//
//
// }
