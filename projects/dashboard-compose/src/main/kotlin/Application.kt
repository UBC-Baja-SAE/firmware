import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.WindowPlacement
import androidx.compose.ui.window.WindowState
import androidx.compose.ui.window.application
import androidx.compose.runtime.LaunchedEffect
import kotlinx.coroutines.Dispatchers 
import kotlinx.coroutines.launch
import kotlinx.coroutines.delay
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json
import org.eclipse.paho.client.mqttv3.MqttClient
import org.eclipse.paho.client.mqttv3.MqttMessage
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import org.baja.dashboard.model.DataRepository
import org.baja.dashboard.model.FullTelemetry
import org.baja.dashboard.model.EcuData
import view.Dashboard

// Constants for Sensitivity (Used in decoding)
const val ACCEL_SENS = 2048.0
const val GYRO_SENS = 16.4
const val SUSP_SCALE = 1000.0

fun main() = application {
    // 1. Initialize Repository & Buffer
    val repository = DataRepository() 
    repository.initBuffer(DataRepository.rawBuffer) // <--- CRITICAL: Links C++ to Kotlin Memory

    // 2. Setup MQTT (Use unique ID to prevent conflicts)
    val clientId = "BajaDash_${System.currentTimeMillis()}"
    val mqttClient = MqttClient("tcp://127.0.0.1:1883", clientId, MemoryPersistence())

    Window(
        onCloseRequest = ::exitApplication,
        state = WindowState(placement = WindowPlacement.Fullscreen),
        undecorated = true,
        alwaysOnTop = true,
    ) {
        // 3. Start Background Services
        LaunchedEffect(Unit) {
            // Thread A: Start C++ Native Loop
            launch(Dispatchers.IO) {
                println("DEBUG: Starting Native Bridge...")
                repository.start() 
            }

            // Thread B: Connect to MQTT Broker
            launch(Dispatchers.IO) {
                try {
                    println("DEBUG: Connecting to MQTT...")
                    mqttClient.connect() 
                    println("DEBUG: MQTT Connected Successfully!")
                } catch (e: Exception) {
                    println("ERROR: MQTT Connection Failed - ${e.message}")
                }
            }
        }

        // 4. Main Telemetry Loop (Polls Buffer -> Publishes MQTT)
        LaunchedEffect(Unit) {
            while (true) {
                if (mqttClient.isConnected) {
                    
                    // A. FETCH RAW DATA (Directly from Shared Memory)
                    // We use getRawData(ID) which is O(1) fast
                    val flAccelRaw = repository.getRawData(0x100)
                    val flGyroRaw  = repository.getRawData(0x101)
                    val flSuspRaw  = repository.getRawData(0x102)

                    val frAccelRaw = repository.getRawData(0x110)
                    val frGyroRaw  = repository.getRawData(0x111)
                    val frSuspRaw  = repository.getRawData(0x112)

                    val rlAccelRaw = repository.getRawData(0x120)
                    val rlGyroRaw  = repository.getRawData(0x121)
                    val rlSuspRaw  = repository.getRawData(0x122)

                    val rrAccelRaw = repository.getRawData(0x130)
                    val rrGyroRaw  = repository.getRawData(0x131)
                    val rrSuspRaw  = repository.getRawData(0x132)

                    // B. CONSTRUCT TELEMETRY OBJECT
                    val data = FullTelemetry(
                        timestamp = System.currentTimeMillis(),
                        speed = DataRepository.decodeRawLE(repository.getRawData(0x201)), 
                        rpm = DataRepository.decodeRawLE(repository.getRawData(0x200)),

                        fl = EcuData(
                            accel = DataRepository.decodeImu(flAccelRaw, ACCEL_SENS),
                            gyro = DataRepository.decodeImu(flGyroRaw, GYRO_SENS),
                            suspension = DataRepository.decodePot(flSuspRaw)
                        ),
                        
                        fr = EcuData(
                            accel = DataRepository.decodeImu(frAccelRaw, ACCEL_SENS),
                            gyro = DataRepository.decodeImu(frGyroRaw, GYRO_SENS),
                            suspension = DataRepository.decodePot(frSuspRaw)
                        ),
                        
                        rl = EcuData(
                            accel = DataRepository.decodeImu(rlAccelRaw, ACCEL_SENS),
                            gyro = DataRepository.decodeImu(rlGyroRaw, GYRO_SENS),
                            suspension = DataRepository.decodePot(rlSuspRaw)
                        ),

                        rr = EcuData(
                            accel = DataRepository.decodeImu(rrAccelRaw, ACCEL_SENS),
                            gyro = DataRepository.decodeImu(rrGyroRaw, GYRO_SENS),
                            suspension = DataRepository.decodePot(rrSuspRaw)
                        )
                    )
                    
                    // C. PUBLISH
                    val json = Json.encodeToString(data)
                    mqttClient.publish("baja/telemetry", MqttMessage(json.toByteArray()).apply { qos = 0 })
                }
                delay(50) // 20Hz Update Rate
            }
        }

        Box(
            modifier = Modifier
                .background(Color.Black)
                .fillMaxSize()
        ) {
            Dashboard() 
        }
    }
}