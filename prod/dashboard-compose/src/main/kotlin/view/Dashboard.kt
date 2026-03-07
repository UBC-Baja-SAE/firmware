package view

import androidx.compose.runtime.setValue
import androidx.compose.runtime.remember
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.animation.core.*
import kotlinx.coroutines.delay
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import org.baja.dashboard.view.gauge.LinearGauge
import org.baja.dashboard.view.gauge.RadialGauge
import org.baja.dashboard.viewmodel.DataViewModel

/**
 * The top level `Composable` element in the user-interface.
 */
@Composable
fun Dashboard() {
    val speed by DataViewModel.getSpeed().collectAsState()
    val rpm by DataViewModel.getRPM().collectAsState()

    // Hard-coded simulation state
    var targetFuel by remember { mutableStateOf(1.0f) }
    var targetTemp by remember { mutableStateOf(45.0f) }

    // Simulation loop: Drains fuel and jitters temp
    LaunchedEffect(Unit) {
        while (true) {
            delay(2000) // Update every 2 seconds
            
            // Slowly drain fuel, reset to 1.0 if it hits empty for demo purposes
            targetFuel = if (targetFuel.toFloat() > 0.05f) targetFuel - 0.02f else 1.0f
            
            // Jitter temperature between 40 and 60
            targetTemp = (40..60).random().toFloat()
        }
    }

    // Animation smoothing
    val animatedFuel by animateFloatAsState(
        targetValue = targetFuel,
        animationSpec = tween(durationMillis = 1500, easing = LinearEasing),
        label = "fuelAnimation"
    )

    val animatedTemp by animateFloatAsState(
        targetValue = targetTemp,
        animationSpec = spring(
            dampingRatio = Spring.DampingRatioMediumBouncy,
            stiffness = Spring.StiffnessLow
        ),
        label = "tempAnimation"
    )

    RadialGauge(
        Modifier.offset(200.dp, 120.dp), 
        speed.toFloat(), 60f, true, "km/h"
    )
    RadialGauge(
        Modifier.offset(680.dp, 120.dp),
        (rpm / 1000.0).toFloat(),
        6f, 
        false, 
        "rpm (x1000)"
    )
    LinearGauge(Modifier.offset(50.dp, 150.dp),
        animatedFuel, 1f, "images/fuel.png", Pair('E', 'F')
    )
    LinearGauge(Modifier.offset(125.dp, 150.dp),
        animatedTemp, 100f, "images/temp.png", Pair('C', 'H')
    )
    Image(
        painter = painterResource("images/logo.png"),
        contentDescription = "",
        modifier = Modifier
            .size(200.dp)
            .offset(540.dp, 50.dp)
    )
}
