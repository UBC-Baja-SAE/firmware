package org.baja.dashboard.viewmodel

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import org.baja.dashboard.model.Data
import org.baja.dashboard.model.DataRepository

object DataViewModel {
    private val repository = DataRepository()

    private val speed = MutableStateFlow(0.0)

    private val temperature = MutableStateFlow(0.0)

    private val rpm = MutableStateFlow(0.0)

    private val fuel = MutableStateFlow(0.0)

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

    private suspend fun startDataCollection() {
        while (true) {
            fetchData()

            delay(1000 / 30) // 30 fps
        }
    }

    private fun fetchData() {
        speed.value = repository.get(Data.Speed.id)
        temperature.value = repository.get(Data.Temperature.id)
        rpm.value = repository.get(Data.RPM.id)
        fuel.value = repository.get(Data.Fuel.id)
    }
}