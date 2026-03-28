#ifndef DASHBOARD_QT_CAN_BRIDGE_H
#define DASHBOARD_QT_CAN_BRIDGE_H

#include <stdint.h>
#include <atomic>

// Array to store the last received data for each CAN ID (up to 2048 IDs)
extern volatile uint64_t observed_data[2048];

/**
 * @brief Observes the last received CAN message with the given id and converts it to the appropriate type.
 * @param id    the id of the CAN message to observe.
 * @return the data received from the CAN message as a double.
 */
double getData(int id);

/**
 * @brief Starts the CAN reader thread (SocketCAN or UART).
 */
void startCanBridge();

/**
 * @brief Stops the CAN reader thread.
 */
void stopCanBridge();

/**
 * @brief Manually inject a CAN frame (for testing).
 * @param can_id The CAN ID
 * @param data Pointer to the data bytes
 * @param len Length of data
 */
void injectCanFrame(int can_id, uint8_t* data, int len);

#endif //DASHBOARD_QT_CAN_BRIDGE_H