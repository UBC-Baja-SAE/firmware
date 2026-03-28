#ifndef DASHBOARD_QT_CAN_PROCESSOR_H
#define DASHBOARD_QT_CAN_PROCESSOR_H

#include <stdint.h>

/**
 * @brief Process a CAN frame and update DataManager.
 * @param can_id The CAN ID
 * @param data Pointer to the CAN frame data
 */
void processIncomingCanFrame(int can_id, uint8_t* data);

/**
 * @brief Start the CAN processor thread that polls observed_data and updates DataManager.
 */
void startCanProcessor();

/**
 * @brief Stop the CAN processor thread.
 */
void stopCanProcessor();

#endif //DASHBOARD_QT_CAN_PROCESSOR_H