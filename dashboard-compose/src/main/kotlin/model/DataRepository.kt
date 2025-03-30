package org.baja.dashboard.model

class DataRepository() {
    external fun test()

    companion object {
        init {
            System.loadLibrary("jni_bridge")
        }
    }
}