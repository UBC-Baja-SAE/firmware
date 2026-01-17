#include "control.h"

extern ADC_HandleTypeDef hadc1;
extern FDCAN_HandleTypeDef hfdcan1;

volatile uint32_t measured_speedometer_frequency;
volatile uint32_t measured_tachometer_frequency;



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

// void SendSpeedometerOnCan(uint32_t can_id) {
//     uint8_t data[2];
//     uint32_t freq_to_send = measured_speedometer_frequency;
//
//     // Split 32-bit integer into 2 bytes (Little Endian)
//     uint16_t val = (uint16_t)freq_to_send;
//     data[0] = val & 0xFF;
//     data[1] = (val >> 8) & 0xFF;
//
//     // Send 2 bytes
//     CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_2);
// }

void SendSpeedometerOnCan(uint32_t can_id) {
    // 1. Create a Union to swap between Double and Bytes
    union {
        double value;
        uint8_t bytes[8];
    } converter;

    // 2. Cast the raw frequency directly to a double
    converter.value = (double)measured_speedometer_frequency;

    // 3. Send the 8 bytes
    CAN_Transmit(can_id, converter.bytes, FDCAN_DLC_BYTES_8);
}


// void SendTachometerOnCan(uint32_t can_id) {
//     uint8_t data[2];
//     uint32_t freq_to_send = measured_tachometer_frequency;
//     //uint32_t freq_to_send = 16;
//
//     // Split 32-bit integer into 2 bytes (Little Endian)
//     uint16_t val = (uint16_t)freq_to_send;
//     data[0] = val & 0xFF;
//     data[1] = (val >> 8) & 0xFF;
//
//     // Send 2 bytes
//     CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_2);
// }


void SendTachometerOnCan(uint32_t can_id) {
    // 1. Create a Union to swap between Double and Bytes
    union {
        double value;
        uint8_t bytes[8];
    } converter;

    measured_tachometer_frequency = (double)measured_tachometer_frequency;

    // 2. Cast the raw frequency directly to a double
    converter.value = (double)measured_tachometer_frequency;

    // 3. Send the 8 bytes
    CAN_Transmit(can_id, converter.bytes, FDCAN_DLC_BYTES_8);
}
