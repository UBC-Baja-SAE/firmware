#include "control.h"

extern FDCAN_HandleTypeDef hfdcan1;
//extern ADC_HandleTypeDef hadc1;

//static void CAN_Transmit(uint32_t id, uint8_t *data, uint32_t len) {
//    FDCAN_TxHeaderTypeDef tx_header = {
//        .Identifier         = id,
//        .IdType             = FDCAN_STANDARD_ID,
//        .TxFrameType        = FDCAN_DATA_FRAME,
//        .DataLength         = len,
//        .ErrorStateIndicator= FDCAN_ESI_ACTIVE,
//        .BitRateSwitch      = FDCAN_BRS_OFF,
//        .FDFormat           = FDCAN_CLASSIC_CAN,
//        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
//        .MessageMarker      = 0
//    };
//
//    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, data) != HAL_OK) {
//        Error_Handler();
//    }
//}

void SendRPMOnCan(uint32_t can_id)
{
    (void) can_id;
}

void SendSpeedOnCan(uint32_t can_id)
{
    (void) can_id;
}

void SendFuelOnCan(uint32_t can_id)
{
    (void) can_id;
}

void SendTempOnCan(uint32_t can_id)
{
    (void) can_id;
}
