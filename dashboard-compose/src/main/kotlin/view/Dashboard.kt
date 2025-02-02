package view

import androidx.compose.foundation.layout.absoluteOffset
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay
import org.baja.dashboard.view.gauge.radial.RadialGauge

/**
 * The top level `Composable` element in the user-interface.
 */
@Composable
fun Dashboard() {
    var value by remember { mutableStateOf(50f) }
    LaunchedEffect(Unit) {
        while (true) {
            value = ((value + 10f) % 100f)
            delay(100)
        }
    }
    RadialGauge(
        Modifier.absoluteOffset(
            400.dp, 120.dp
        ),
        value, 100f)
}