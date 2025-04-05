package org.baja.dashboard.model

/**
 * A data classification for data values that are consumed in the Dashboard
 * application.
 * @property id the CAN bus identifier associated with each data value.
 */
enum class Data(val id: Int) {
    Speed(1),
    Temperature(2),
    RPM(3),
    Fuel(4)
}