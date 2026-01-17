package org.baja.dashboard.model

class DataRepository {
    companion object {
        init {
            System.loadLibrary("can_processor")
        }
    }

    external fun getSpeed(): Double
    external fun getTemperature(): Double
    external fun getRPM(): Double
    external fun getFuel(): Double

    // --- Front Left (FL) ECU ---
    external fun getFLAccel(): Double
    external fun getFLGyro(): Double
    external fun getFLSuspension(): Double
    external fun getFLStrainL(): Double
    external fun getFLStrainR(): Double

    // --- Front Right (FR) ECU ---
    external fun getFRAccel(): Double
    external fun getFRGyro(): Double
    external fun getFRSuspension(): Double
    external fun getFRStrainL(): Double
    external fun getFRStrainR(): Double

    // --- Rear Left (RL) ECU ---
    external fun getRLAccel(): Double
    external fun getRLGyro(): Double
    external fun getRLSuspension(): Double
    external fun getRLStrainL(): Double
    external fun getRLStrainR(): Double

    // --- Rear Right (RR) ECU ---
    external fun getRRAccel(): Double
    external fun getRRGyro(): Double
    external fun getRRSuspension(): Double
    external fun getRRStrainL(): Double
    external fun getRRStrainR(): Double

    // --- Raspberry Pi Local Sensors ---
    external fun getPiAccel(): Double
    external fun getPiGyro(): Double

    external fun start()
}