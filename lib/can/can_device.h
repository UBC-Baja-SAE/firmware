/**
 * @file can_device.h
 * @brief Represents the physical layer as an abstract representation of a
 * physical CAN device, such as a sensor or actuator
 */

#ifndef CAN_DEVICE_H
#define CAN_DEVICE_H

/**
 * Interface that represents a device that communicates on the CAN bus.
 */
class CANDevice 
{
public:
    /**
     * @brief Represents the 11-bit CAN identifier and the emssage priority
     */
    virtual uint8_t id;

    /**
     * @brief Transmits the given data onto the CAN bus
     * 
     * @param data The 64-bit data to be transmitted as a CAN message
     */
    virtual void sendData(uint64_t data);

    /**
     * @brief Reads the CAN bus for messages
     * 
     * @return The 64-bit data currently being transmitted on the CAN bus
     */
    virtual uint64_t receiveData();
}

#endif // CAN_DEVICE_H
