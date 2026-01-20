#include "control.h"

extern ADC_HandleTypeDef hadc1;
extern FDCAN_HandleTypeDef hfdcan1;

volatile float measured_speedometer_frequency;
volatile float measured_tachometer_frequency;


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

void SendSpeedOnCan(uint32_t can_id) {
    uint8_t data[4] = {0};

    // Create a local copy to avoid interrupt changing it mid-read (atomic access for 32-bit usually fine, but safer)
    uint32_t freq_to_send = measured_speedometer_frequency;

    // Split 32-bit integer into 4 bytes (Little Endian)
    data[0] = (uint8_t)(freq_to_send & 0xFF);
    data[1] = (uint8_t)((freq_to_send >> 8) & 0xFF);
    data[2] = (uint8_t)((freq_to_send >> 16) & 0xFF);
    data[3] = (uint8_t)((freq_to_send >> 24) & 0xFF);

    // Send 4 bytes
    CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_4);
}

void SendTachometerOnCan(uint32_t can_id) {
    uint8_t data[4] = {0};

    // Create a local copy to avoid interrupt changing it mid-read (atomic access for 32-bit usually fine, but safer)
    uint32_t freq_to_send = measured_tachometer_frequency;

    // Split 32-bit integer into 4 bytes (Little Endian)
    data[0] = (uint8_t)(freq_to_send & 0xFF);
    data[1] = (uint8_t)((freq_to_send >> 8) & 0xFF);
    data[2] = (uint8_t)((freq_to_send >> 16) & 0xFF);
    data[3] = (uint8_t)((freq_to_send >> 24) & 0xFF);

    // Send 4 bytes
    CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_4);
}
