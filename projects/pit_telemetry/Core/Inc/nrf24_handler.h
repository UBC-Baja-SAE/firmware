#ifndef DASHBOARD_QT_NRF24_HANDLER_H
#define DASHBOARD_QT_NRF24_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

// ─── Packet Types ─────────────────────────────────────────────────────────────
// First byte of every 32-byte payload identifies the packet type.
// Car → Pi: ECU corners, powertrain, GPS
// Pi → Car: commands (expand as needed)

typedef enum __attribute__((packed)) {
    PKT_ECU_FL      = 0x01,
    PKT_ECU_FR      = 0x02,
    PKT_ECU_RL      = 0x03,
    PKT_ECU_RR      = 0x04,
    PKT_POWERTRAIN  = 0x05,
    PKT_GPS         = 0x06,
    PKT_CMD         = 0x10,   // Pi → Car commands (reserved for future use)
} NrfPacketType;

// ─── Payload Structs (must be <= 32 bytes including packet_type) ───────────────

typedef struct __attribute__((packed)) {
    uint8_t  packet_type;           // 1  — NrfPacketType
    int16_t  accel_x, accel_y, accel_z;  // 6  — raw ICM-42670 counts
    int16_t  gyro_x,  gyro_y,  gyro_z;   // 6  — raw ICM-42670 counts
    int16_t  strain_l, strain_r;    // 4  — signed ADC counts
    uint16_t travel;                // 2  — unsigned ADC counts
    uint8_t  _pad[13];              // 13 — reserved / future sensors
} NrfEcuPayload;                    // = 32 bytes

typedef struct __attribute__((packed)) {
    uint8_t  packet_type;           // 1
    uint32_t tach;                  // 4
    uint32_t speedo;                // 4
    uint32_t temp;                  // 4
    uint32_t fuel;                  // 4
    uint8_t  _pad[15];              // 15
} NrfPowertrainPayload;             // = 32 bytes

typedef struct __attribute__((packed)) {
    uint8_t  packet_type;           // 1
    float    latitude;              // 4
    float    longitude;             // 4
    float    gps_speed;             // 4
    uint8_t  has_fix;               // 1
    uint8_t  _pad[18];              // 18
} NrfGpsPayload;                    // = 32 bytes

typedef struct __attribute__((packed)) {
    uint8_t  packet_type;           // 1  — PKT_CMD
    uint8_t  command_id;            // 1  — expand enum as commands are defined
    uint8_t  payload[30];           // 30 — command-specific data
} NrfCmdPayload;                    // = 32 bytes

// Union for easy casting of raw 32-byte buffer
typedef union {
    uint8_t             raw[32];
    NrfEcuPayload       ecu;
    NrfPowertrainPayload powertrain;
    NrfGpsPayload       gps;
    NrfCmdPayload       cmd;
} NrfPacket;

// ─── Public API ───────────────────────────────────────────────────────────────

#ifdef __cplusplus
extern "C" {
#endif

bool startNrf24();
void stopNrf24();
bool sendCommand(uint8_t command_id, const uint8_t* data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif // DASHBOARD_QT_NRF24_HANDLER_H