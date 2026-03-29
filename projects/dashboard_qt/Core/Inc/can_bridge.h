#ifndef DASHBOARD_QT_CAN_BRIDGE_H
#define DASHBOARD_QT_CAN_BRIDGE_H

#include <stdint.h>

// Mirror of the last received payload for each CAN ID (legacy UI support)
extern volatile uint64_t observed_data[2048];

/**
 * @brief Starts the CAN reader thread on can0, falling back to vcan0.
 *        Dispatches frames to DataManager for:
 *          - Powertrain  (tach, speed, temp, fuel)
 *          - Suspension  (travel per corner)
 *          - Strain      (strain_l, strain_r per corner)
 *          - IMU accel   (x, y, z per corner)
 *          - IMU gyro    (x, y, z per corner)
 *          - GPS         (latitude, longitude, speed, fix)
 */
void startCanBridge();

/**
 * @brief Stops the CAN reader thread and joins it.
 */
void stopCanBridge();

/**
 * @brief Injects a CAN frame manually (for testing/simulation).
 * @param can_id  CAN ID (0–2047)
 * @param data    Pointer to frame payload
 * @param len     Payload length in bytes (clamped to 8)
 */
void injectCanFrame(int can_id, uint8_t* data, int len);

#endif //DASHBOARD_QT_CAN_BRIDGE_H