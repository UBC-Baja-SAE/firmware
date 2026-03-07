import org.baja.dashboard.model.DataRepository
import androidx.compose.foundation.layout.*
import androidx.compose.material.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.pow

/**
 * Visual module to display converted IEEE Half-Precision values from CAN IDs 110 and 111.
 * Designed to be dropped into the existing Compose UI layout.
 */
@Composable
fun CanValueModule(canDataMap: Map<Int, Long>) {
    // Extracting the 64-bit Little-Endian raw data from the map
    val raw110 = canDataMap[110] ?: 0L
    val raw111 = canDataMap[111] ?: 0L

    // According to the Little-Endian format, the first 16 bits are the relevant value
    val value110 = convertHalfToFraction(raw110.toInt() and 0xFFFF)
    val value111 = convertHalfToFraction(raw111.toInt() and 0xFFFF)

    Column(
        modifier = Modifier
            .padding(16.dp)
            .fillMaxWidth()
    ) {
        Text("CAN TELEMETRY", color = Color.Gray, fontSize = 12.sp, fontWeight = FontWeight.Bold)
        Spacer(modifier = Modifier.height(8.dp))

        ValueRow(label = "ID 110 Value:", value = value110)
        ValueRow(label = "ID 111 Value:", value = value111)
    }
}

@Composable
private fun ValueRow(label: String, value: String) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label, color = Color.White, fontSize = 18.sp)
        Text(value, color = Color.Blue, fontSize = 18.sp, fontWeight = FontWeight.Bold)
    }
}

/**
 * Converts a 16-bit IEEE 754 Half-Precision bit pattern to a fractional string.
 */
fun convertHalfToFraction(bits: Int): String {
    val s = (bits shr 15) and 0x1
    val e = (bits shr 10) and 0x1F
    val m = bits and 0x3FF

    return when {
        e == 0 && m == 0 -> "0"
        e == 31 -> if (m == 0) "Infinity" else "NaN"
        else -> {
            val mantissa = if (e == 0) m.toDouble() / 1024.0 else 1.0 + (m.toDouble() / 1024.0)
            val exponent = if (e == 0) -14 else e - 15
            val result = (-1.0).pow(s.toDouble()) * mantissa * 2.0.pow(exponent.toDouble())

            // Converting to a simple fractional representation
            String.format("%.3f", result)
        }
    }
}
