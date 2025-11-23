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

    external fun start()
}