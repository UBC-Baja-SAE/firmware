package view

import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay
import org.baja.dashboard.view.gauge.LinearGauge
import org.baja.dashboard.view.gauge.RadialGauge

/**
 * The top level `Composable` element in the user-interface.
 */
@Composable
fun Dashboard() {
    var value by remember { mutableStateOf(0f) }

    LaunchedEffect(Unit) {
        while (true) {
            value = ((value + 1f) % 100f)
            delay(1000 / 60)
        }
    }
    RadialGauge(
        Modifier.offset(200.dp, 120.dp),
        value, 100f, true, "km/h"
    )
    RadialGauge(
        Modifier.offset(680.dp, 120.dp),
        100f - value, 100f, false, "rpm (x1000)"
    )
    LinearGauge(Modifier.offset(50.dp, 150.dp),
        value, 100f, "images/fuel.png", Pair('E', 'F')
    )
    LinearGauge(Modifier.offset(125.dp, 150.dp),
        value, 100f, "images/temp.png", Pair('C', 'H')
    )
    Image(
        painter = painterResource("images/logo.png"),
        contentDescription = "",
        modifier = Modifier
            .size(200.dp)
            .offset(540.dp, 50.dp)
    )
}