package view

import androidx.compose.foundation.layout.absoluteOffset
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.res.painterResource
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
            value = ((value + 1f) % 100f)
            delay(1000 / 60)
        }
    }
    RadialGauge(
        Modifier.absoluteOffset(
            300.dp, 120.dp
        ),
        value, 100f, true, "Label")

    Image(
        painter = painterResource("images/logo.png"),
        contentDescription = "",
        modifier = Modifier.size(100.dp)
    )
}