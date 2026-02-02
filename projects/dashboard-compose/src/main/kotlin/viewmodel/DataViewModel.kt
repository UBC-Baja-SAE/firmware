package org.baja.dashboard.viewmodel

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import org.baja.dashboard.model.DataRepository
import org.baja.dashboard.model.Vec3
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

// Constants
const val ACCEL_SENS = 2048.0 
const val GYRO_SENS = 16.4    

object DataViewModel {
    private var logFile: File? = null

    private val repository = DataRepository()
    
    // Scalar Flows
    private val speed = MutableStateFlow(0.0)
    private val temperature = MutableStateFlow(0.0)
    private val rpm = MutableStateFlow(0.0)
    private val fuel = MutableStateFlow(0.0)

    // Vector Flows (IMU)
    private val flAccel = MutableStateFlow(Vec3(0.0, 0.0, 0.0))
    private val flGyro = MutableStateFlow(Vec3(0.0, 0.0, 0.0))
    private val flSuspension = MutableStateFlow(0.0)
    private val flStrainL = MutableStateFlow(0.0)
    private val flStrainR = MutableStateFlow(0.0)

    private val frAccel = MutableStateFlow(Vec3(0.0, 0.0, 0.0))
    private val frGyro = MutableStateFlow(Vec3(0.0, 0.0, 0.0))
    private val frSuspension = MutableStateFlow(0.0)
    private val frStrainL = MutableStateFlow(0.0)
    private val frStrainR = MutableStateFlow(0.0)

    private val rlAccel = MutableStateFlow(Vec3(0.0, 0.0, 0.0))
    private val rlGyro = MutableStateFlow(Vec3(0.0, 0.0, 0.0))
    private val rlSuspension = MutableStateFlow(0.0)
    private val rlStrainL = MutableStateFlow(0.0)
    private val rlStrainR = MutableStateFlow(0.0)

    private val rrAccel = MutableStateFlow(Vec3(0.0, 0.0, 0.0))
    private val rrGyro = MutableStateFlow(Vec3(0.0, 0.0, 0.0))
    private val rrSuspension = MutableStateFlow(0.0)
    private val rrStrainL = MutableStateFlow(0.0)
    private val rrStrainR = MutableStateFlow(0.0)

    private val piAccel = MutableStateFlow(Vec3(0.0, 0.0, 0.0))
    private val piGyro = MutableStateFlow(Vec3(0.0, 0.0, 0.0))

    init {
        initLogger()

        // We only start the DATA COLLECTION loop.
        // We do NOT start the C++ repository.start() here because Application.kt already does it.
        CoroutineScope(Dispatchers.IO).launch {
            startDataCollection()
        }
    }

    private fun initLogger() {
        try {
            val folder = File("/home/ubcbaja/firmware/logs")
            if (!folder.exists()) folder.mkdirs()

            val timestamp = SimpleDateFormat("yyyyMMdd_HHmm", Locale.US).format(Date())
            logFile = File(folder, "baja_log_$timestamp.csv")

            val header = listOf(
                "Timestamp", "Speed", "RPM", "Temp", "Fuel",
                "FL_Accel_X", "FL_Accel_Y", "FL_Accel_Z", "FL_Susp",
                "FR_Accel_X", "FR_Accel_Y", "FR_Accel_Z", "FR_Susp",
                "RL_Accel_X", "RL_Accel_Y", "RL_Accel_Z", "RL_Susp",
                "RR_Accel_X", "RR_Accel_Y", "RR_Accel_Z", "RR_Susp"
            ).joinToString(",") + "\n"

            logFile?.writeText(header)
        } catch (e: Exception) {
            println("Logger Init Error: ${e.message}")
        }
    }

    // --- GETTERS ---
    fun getSpeed() = speed.asStateFlow()
    fun getRPM() = rpm.asStateFlow()

    private suspend fun startDataCollection() {
        while (true) {
            fetchData()
            delay(1000 / 30) // 30Hz Update Rate
        }
    }

    private fun fetchData() {
        // --- THE FIX: Read from Shared Memory using ID ---
        
        // 1. Scalar Values (Speed/RPM)
        speed.value = DataRepository.decodeRawLE(repository.getRawData(0x201))
        rpm.value = DataRepository.decodeRawLE(repository.getRawData(0x200))

        // 2. Vector Values (IMU)
        flAccel.value = DataRepository.decodeImu(repository.getRawData(0x100), ACCEL_SENS)
        flGyro.value  = DataRepository.decodeImu(repository.getRawData(0x101), GYRO_SENS)
        flSuspension.value = DataRepository.decodePot(repository.getRawData(0x102))

        frAccel.value = DataRepository.decodeImu(repository.getRawData(0x110), ACCEL_SENS)
        frGyro.value  = DataRepository.decodeImu(repository.getRawData(0x111), GYRO_SENS)
        frSuspension.value = DataRepository.decodePot(repository.getRawData(0x112))

        rlAccel.value = DataRepository.decodeImu(repository.getRawData(0x120), ACCEL_SENS)
        rlGyro.value  = DataRepository.decodeImu(repository.getRawData(0x121), GYRO_SENS)
        rlSuspension.value = DataRepository.decodePot(repository.getRawData(0x122))

        rrAccel.value = DataRepository.decodeImu(repository.getRawData(0x130), ACCEL_SENS)
        rrGyro.value  = DataRepository.decodeImu(repository.getRawData(0x131), GYRO_SENS)
        rrSuspension.value = DataRepository.decodePot(repository.getRawData(0x132))

        writeLog()
    }
    
    private fun writeLog() {
        val file = logFile ?: return
        
        // Flatten data for CSV
        val dataPoints = listOf(
            speed.value, rpm.value,
            flAccel.value.x, flAccel.value.y, flAccel.value.z, flSuspension.value,
            frAccel.value.x, frAccel.value.y, frAccel.value.z, frSuspension.value,
            rlAccel.value.x, rlAccel.value.y, rlAccel.value.z, rlSuspension.value,
            rrAccel.value.x, rrAccel.value.y, rrAccel.value.z, rrSuspension.value
        )
        
        val row = (listOf(System.currentTimeMillis()) + dataPoints).joinToString(",") + "\n"
        try { file.appendText(row) } catch (e: Exception) {}
    }
}