package org.baja.dashboard.viewmodel

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import org.baja.dashboard.model.DataRepository

object DataViewModel {
    private val repository = DataRepository()
    private val speed = MutableStateFlow(0.0)
    private val temperature = MutableStateFlow(0.0)
    private val rpm = MutableStateFlow(0.0)
    private val fuel = MutableStateFlow(0.0)

    // --- Front Left ECU Flows ---
    private val flAccel = MutableStateFlow(0.0)
    private val flGyro = MutableStateFlow(0.0)
    private val flSuspension = MutableStateFlow(0.0)
    private val flStrainL = MutableStateFlow(0.0)
    private val flStrainR = MutableStateFlow(0.0)

    // --- Front Right ECU Flows ---
    private val frAccel = MutableStateFlow(0.0)
    private val frGyro = MutableStateFlow(0.0)
    private val frSuspension = MutableStateFlow(0.0)
    private val frStrainL = MutableStateFlow(0.0)
    private val frStrainR = MutableStateFlow(0.0)

    // --- Rear Left ECU Flows ---
    private val rlAccel = MutableStateFlow(0.0)
    private val rlGyro = MutableStateFlow(0.0)
    private val rlSuspension = MutableStateFlow(0.0)
    private val rlStrainL = MutableStateFlow(0.0)
    private val rlStrainR = MutableStateFlow(0.0)

    // --- Rear Right ECU Flows ---
    private val rrAccel = MutableStateFlow(0.0)
    private val rrGyro = MutableStateFlow(0.0)
    private val rrSuspension = MutableStateFlow(0.0)
    private val rrStrainL = MutableStateFlow(0.0)
    private val rrStrainR = MutableStateFlow(0.0)

    // --- Pi IMU Flows ---
    private val piAccel = MutableStateFlow(0.0)
    private val piGyro = MutableStateFlow(0.0)

    init {
        CoroutineScope(Dispatchers.IO).launch {
            startDataCollection()
        }

        CoroutineScope(Dispatchers.IO).launch {
            repository.start()
        }
    }

    fun getSpeed() = speed.asStateFlow()
    fun getTemperature() = temperature.asStateFlow()
    fun getRPM() = rpm.asStateFlow()
    fun getFuel() = fuel.asStateFlow()

    fun getFLAccel() = flAccel.asStateFlow()
    fun getFLGyro() = flGyro.asStateFlow()
    fun getFLSuspension() = flSuspension.asStateFlow()
    fun getFLStrainL() = flStrainL.asStateFlow()
    fun getFLStrainR() = flStrainR.asStateFlow()

    fun getFRAccel() = frAccel.asStateFlow()
    fun getFRGyro() = frGyro.asStateFlow()
    fun getFRSuspension() = frSuspension.asStateFlow()
    fun getFRStrainL() = frStrainL.asStateFlow()
    fun getFRStrainR() = frStrainR.asStateFlow()

    fun getRLAccel() = rlAccel.asStateFlow()
    fun getRLGyro() = rlGyro.asStateFlow()
    fun getRLSuspension() = rlSuspension.asStateFlow()
    fun getRLStrainL() = rlStrainL.asStateFlow()
    fun getRLStrainR() = rlStrainR.asStateFlow()

    fun getRRAccel() = rrAccel.asStateFlow()
    fun getRRGyro() = rrGyro.asStateFlow()
    fun getRRSuspension() = rrSuspension.asStateFlow()
    fun getRRStrainL() = rrStrainL.asStateFlow()
    fun getRRStrainR() = rrStrainR.asStateFlow()

    fun getPiAccel() = piAccel.asStateFlow()
    fun getPiGyro() = piGyro.asStateFlow()

    private suspend fun startDataCollection() {
        while (true) {
            fetchData()

            println("speed: ${speed.value}")

            delay(1000 / 30) // 30 fps
        }
    }

    private fun fetchData() {
        speed.value = repository.getSpeed()
        temperature.value = repository.getTemperature()
        rpm.value = repository.getRPM()
        fuel.value = repository.getFuel()

        // FL
        flAccel.value = repository.getFLAccel()
        flGyro.value = repository.getFLGyro()
        flSuspension.value = repository.getFLSuspension()
        flStrainL.value = repository.getFLStrainL()
        flStrainR.value = repository.getFLStrainR()

        // FR
        frAccel.value = repository.getFRAccel()
        frGyro.value = repository.getFRGyro()
        frSuspension.value = repository.getFRSuspension()
        frStrainL.value = repository.getFRStrainL()
        frStrainR.value = repository.getFRStrainR()

        // RL
        rlAccel.value = repository.getRLAccel()
        rlGyro.value = repository.getRLGyro()
        rlSuspension.value = repository.getRLSuspension()
        rlStrainL.value = repository.getRLStrainL()
        rlStrainR.value = repository.getRLStrainR()

        // RR
        rrAccel.value = repository.getRRAccel()
        rrGyro.value = repository.getRRGyro()
        rrSuspension.value = repository.getRRSuspension()
        rrStrainL.value = repository.getRRStrainL()
        rrStrainR.value = repository.getRRStrainR()

        // Pi
        piAccel.value = repository.getPiAccel()
        piGyro.value = repository.getPiGyro()
    }
}