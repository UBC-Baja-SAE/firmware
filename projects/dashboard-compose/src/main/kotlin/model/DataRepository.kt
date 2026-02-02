package org.baja.dashboard.model
import androidx.compose.runtime.mutableStateMapOf
import kotlinx.serialization.Serializable
import java.nio.ByteBuffer
import java.nio.ByteOrder

class DataRepository {
    companion object {
        init {
            System.loadLibrary("can_processor")
        }
        
        val rawBuffer: ByteBuffer = ByteBuffer.allocateDirect(2048 * 8)
            .order(ByteOrder.LITTLE_ENDIAN) 

        fun decodeImu(raw: Long?, sensitivity: Double): Vec3 {
            if (raw == null) return Vec3(0.0, 0.0, 0.0)
            val bytes = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(raw).array()
            val buffer = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN)
            
            val x = buffer.getShort(0).toDouble() / sensitivity
            val y = buffer.getShort(2).toDouble() / sensitivity
            val z = buffer.getShort(4).toDouble() / sensitivity
            return Vec3(x, y, z)
        }

        fun decodePot(raw: Long?): Double {
            if (raw == null) return 0.0
            val bytes = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(raw).array()
            val buffer = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN)
            val value = buffer.getShort(0).toInt() and 0xFFFF 
            return value / 100.0 // SUSP_SCALE
        }

        fun decodeRaw(raw: Long?): Double {
            if (raw == null) return 0.0
                val bytes = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(raw).array()
                val buffer = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN)
                return (buffer.getShort(0).toInt() and 0xFFFF).toDouble()
            }


fun decodeRawLE(raw: Long?): Double {
    if (raw == null) return 0.0
    val bytes = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(raw).array()
    
    // CHANGE: Use LITTLE_ENDIAN here to swap the byte processing order
    val buffer = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
    
    // Read first 2 bytes
    return (buffer.getShort(0).toInt() and 0xFFFF).toDouble()
}
    }

    // Connects buffer to C++
    external fun initBuffer(buffer: ByteBuffer)
    external fun start()

    // Fast Reader
    fun getRawData(canId: Int): Long {
        return rawBuffer.getLong(canId * 8)
    }

    // Legacy getters
    external fun getSpeed(): Double
    external fun getRPM(): Double
    external fun getTemperature(): Double
    external fun getFuel(): Double

    // Legacy Stubs (to satisfy old code calls if any remain)
    external fun getFLAccel(): Double
    external fun getFLGyro(): Double
    external fun getFLSuspension(): Double
    external fun getFLStrainL(): Double
    external fun getFLStrainR(): Double

    external fun getFRAccel(): Double
    external fun getFRGyro(): Double
    external fun getFRSuspension(): Double
    external fun getFRStrainL(): Double
    external fun getFRStrainR(): Double

    external fun getRLAccel(): Double
    external fun getRLGyro(): Double
    external fun getRLSuspension(): Double
    external fun getRLStrainL(): Double
    external fun getRLStrainR(): Double

    external fun getRRAccel(): Double
    external fun getRRGyro(): Double
    external fun getRRSuspension(): Double
    external fun getRRStrainL(): Double
    external fun getRRStrainR(): Double

    external fun getPiAccel(): Double
    external fun getPiGyro(): Double
}

@Serializable
data class FullTelemetry(
    val timestamp: Long,
    val speed: Double,
    val rpm: Double,
    val fl: EcuData,
    val fr: EcuData,
    val rl: EcuData,
    val rr: EcuData
)

@Serializable
data class Vec3(
    val x: Double,
    val y: Double,
    val z: Double
)

@Serializable
data class EcuData(
    val accel: Vec3,
    val gyro: Vec3,
    val suspension: Double
)