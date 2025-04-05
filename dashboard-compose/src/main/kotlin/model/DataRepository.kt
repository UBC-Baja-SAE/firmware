package org.baja.dashboard.model

class DataRepository {
    companion object {
        init {
            System.loadLibrary("jni_bridge")
        }
    }

    external fun get(id: Int): Double
}