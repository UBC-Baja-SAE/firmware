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

    private val _speed = MutableStateFlow(0.0)
    val speed = _speed.asStateFlow()

    private val _temperature = MutableStateFlow(0.0)
    val temperature = _temperature.asStateFlow()

    private val _rpm = MutableStateFlow(0.0)
    val rpm = _rpm.asStateFlow()

    private val _fuel = MutableStateFlow(0.0)
    val fuel = _fuel.asStateFlow()

    init {
        CoroutineScope(Dispatchers.IO).launch {
            startDataCollection()
        }
    }

    private suspend fun startDataCollection() {
        while (true) {
            fetchData()

            delay(3000) // runs at 40 fps
        }
    }

    private fun fetchData() {
        _speed.value = repository.get(Data.Speed.id)
        _temperature.value = repository.get(Data.Temperature.id)
        _rpm.value = repository.get(Data.RPM.id)
        _fuel.value = repository.get(Data.Fuel.id)
    }
}