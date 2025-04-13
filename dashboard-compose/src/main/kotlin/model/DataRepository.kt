package org.baja.dashboard.model

class DataRepository {
    companion object {
        init {
            System.loadLibrary("can_processor")
        }
    }

    external fun get(id: Int): Double

    external fun start()
}