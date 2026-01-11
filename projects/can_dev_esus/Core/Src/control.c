#include "control.h"

extern ADC_HandleTypeDef hadc1;
extern FDCAN_HandleTypeDef hfdcan1;

volatile uint32_t measured_frequency;


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

float VoltageToPosition(float voltage)
{
    return X_EXTEND_CM + (voltage * (X_EXTEND_CM - X_COMP_CM) / (V_EXTEND - V_COMP));
}

void SendPotOnCan(uint32_t can_id)
{
    const int samples = 10;
    float posSum = 0;

    for (int i = 0; i < samples; i++)
    {
        HAL_ADC_Start(&hadc1);

        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
        {
            uint32_t adcRaw = HAL_ADC_GetValue(&hadc1); 
            float voltage = ((float)adcRaw * 5.0f) / 65535.0f; 
            float position = VoltageToPosition(voltage);
            posSum += position;
        }

        HAL_ADC_Stop(&hadc1);
        HAL_Delay(1); 
    }

    float posAvg = posSum / samples;
    uint16_t posCan = (uint16_t)(posAvg * 100.0f); 

    pot_tx_data[0] = (posCan >> 8) & 0xFF;
    pot_tx_data[1] = posCan & 0xFF;

    CAN_Transmit(can_id, pot_tx_data, FDCAN_DLC_BYTES_2);

    // FDCAN_TxHeaderTypeDef tx_header = {
    //     .Identifier         = can_id,
    //     .IdType             = FDCAN_STANDARD_ID,
    //     .TxFrameType        = FDCAN_DATA_FRAME,
    //     .DataLength         = FDCAN_DLC_BYTES_2,
    //     .ErrorStateIndicator= FDCAN_ESI_ACTIVE,
    //     .BitRateSwitch      = FDCAN_BRS_OFF,
    //     .FDFormat           = FDCAN_CLASSIC_CAN,
    //     .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
    //     .MessageMarker      = 0
    // };

    // if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, pot_tx_data) != HAL_OK)
    // {
    //     Error_Handler();
    // }
}


void SendAccelOnCan(uint32_t can_id) {
    uint8_t data[6] = {0}; 
    // data[0], [1] = X | data[2], [3] = Y | data[4], [5] = Z
    CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_6);
}

void SendGyroOnCan(uint32_t can_id) {
    uint8_t data[6] = {0};
    CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_6);
}

// 4. Strain Gauges - 2 Bytes expected
void SendStrainOnCan(uint32_t can_id) {
    uint8_t data[2] = {0};
    CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_2);
}

void SendFrequencyOnCan(uint32_t can_id) {
    uint8_t data[4] = {0};

    // Create a local copy to avoid interrupt changing it mid-read (atomic access for 32-bit usually fine, but safer)
    uint32_t freq_to_send = measured_frequency;

    // Split 32-bit integer into 4 bytes (Little Endian)
    data[0] = (uint8_t)(freq_to_send & 0xFF);
    data[1] = (uint8_t)((freq_to_send >> 8) & 0xFF);
    data[2] = (uint8_t)((freq_to_send >> 16) & 0xFF);
    data[3] = (uint8_t)((freq_to_send >> 24) & 0xFF);

    // Send 4 bytes
    CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_4);
}