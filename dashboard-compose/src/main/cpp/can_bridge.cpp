#include <jni.h>
#include <stdint.h>
#include <string.h>
#include "can_bridge.h"
#include "can_processor.h"
#include "can_id.h"

/**
 * @brief Observes the last received CAN message with the given id.
 * @param id    the id of the CAN message to observe.
 * @return the data received from the CAN message. 
 */
double getData(int id)
{
    uint64_t data = observed_data[id];

    double result;
    memcpy(&result, &data, sizeof(data));
    return result;
}

getSpeed
{
    return (jdouble) getData(speedometer_id);
}

getTemperature
{
    return (jdouble) getData(thermometer_id);
}

getRPM
{
    return (jdouble) getData(tachometer_id);
}

getFuel
{
    return (jdouble) getData(fuel_sensor_id);
}

startProcessor
{
    start();
}