#include "control.h"

// Hardware Handles
extern ADC_HandleTypeDef hadc1;  // Used for Potentiometer
extern ADC_HandleTypeDef hadc2;  // Used for Strain Gauge (Assumed based on main.c)
extern FDCAN_HandleTypeDef hfdcan1;
extern I2C_HandleTypeDef hi2c1;  // Used for IMU

uint8_t pot_tx_data[2];

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

// Initialization for ICM-42670-P
void IMU_Init(void) {
    // Wake up: Enable both Accel and Gyro in Low Noise mode
    uint8_t pwr_mgmt = 0x0F;
    HAL_I2C_Mem_Write(&hi2c1, ICM_ADDR, PWR_MGMT0, I2C_MEMADD_SIZE_8BIT, &pwr_mgmt, 1, 100);
}

float VoltageToPosition(float voltage)
{
    return X_EXTEND_CM + (voltage * (X_EXTEND_CM - X_COMP_CM) / (V_EXTEND - V_COMP));
}

void SendPotOnCan(uint32_t can_id)
{
    const int samples = 10;
    float posSum = 0;
    ADC_ChannelConfTypeDef sConfig = {0};

    // Reconfigure ADC1 to the Potentiometer channel (PB1)
    sConfig.Channel = ADC_CHANNEL_5;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5; // Increased for stability
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    for (int i = 0; i < samples; i++)
    {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
        {
            uint32_t adcRaw = HAL_ADC_GetValue(&hadc1); 
            float voltage = ((float)adcRaw * 3.3f) / 65535.0f; // Note: H7 is usually 3.3V
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

void SendAccelOnCan(uint32_t can_id) {
    uint8_t raw_data[6];
    // Read 6 bytes starting from ACCEL_DATA_X1 (0x0B)
    if (HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, ACCEL_DATA_START, I2C_MEMADD_SIZE_8BIT, raw_data, 6, 100) == HAL_OK) {
        CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);
    }
}

void SendGyroOnCan(uint32_t can_id) {
    uint8_t raw_data[6];
    // Read 6 bytes starting from GYRO_DATA_X1 (0x11)
    if (HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, GYRO_DATA_START, I2C_MEMADD_SIZE_8BIT, raw_data, 6, 100) == HAL_OK) {
        CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);
    }
}

void SendStrainOnCan(uint32_t can_id, uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    uint16_t strain_val = 0;

    // Dynamically switch ADC1 to the requested Strain Gauge channel
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;

    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        strain_val = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    uint8_t data[2] = { (strain_val >> 8) & 0xFF, strain_val & 0xFF };
    CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_2);
}
