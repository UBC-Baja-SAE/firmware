package view

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
    val temp by DataViewModel.getTemperature().collectAsState()
    val rpm by DataViewModel.getRPM().collectAsState()
    val fuel by DataViewModel.getFuel().collectAsState()

    // --- Front Left (FL) ---
    val flAccel by DataViewModel.getFLAccel().collectAsState()
    val flGyro by DataViewModel.getFLGyro().collectAsState()
    val flSuspension by DataViewModel.getFLSuspension().collectAsState()
    val flStrainL by DataViewModel.getFLStrainL().collectAsState()
    val flStrainR by DataViewModel.getFLStrainR().collectAsState()

    // --- Front Right (FR) ---
    val frAccel by DataViewModel.getFRAccel().collectAsState()
    val frGyro by DataViewModel.getFRGyro().collectAsState()
    val frSuspension by DataViewModel.getFRSuspension().collectAsState()
    val frStrainL by DataViewModel.getFRStrainL().collectAsState()
    val frStrainR by DataViewModel.getFRStrainR().collectAsState()

    // --- Rear Left (RL) ---
    val rlAccel by DataViewModel.getRLAccel().collectAsState()
    val rlGyro by DataViewModel.getRLGyro().collectAsState()
    val rlSuspension by DataViewModel.getRLSuspension().collectAsState()
    val rlStrainL by DataViewModel.getRLStrainL().collectAsState()
    val rlStrainR by DataViewModel.getRLStrainR().collectAsState()

    // --- Rear Right (RR) ---
    val rrAccel by DataViewModel.getRRAccel().collectAsState()
    val rrGyro by DataViewModel.getRRGyro().collectAsState()
    val rrSuspension by DataViewModel.getRRSuspension().collectAsState()
    val rrStrainL by DataViewModel.getRRStrainL().collectAsState()
    val rrStrainR by DataViewModel.getRRStrainR().collectAsState()

    // --- Pi IMU ---
    val piAccel by DataViewModel.getPiAccel().collectAsState()
    val piGyro by DataViewModel.getPiGyro().collectAsState()

    RadialGauge(
        Modifier.offset(200.dp, 120.dp), 
        speed.toFloat(), 70f, true, "km/h"
    )
    RadialGauge(
        Modifier.offset(680.dp, 120.dp),
        rpm.toFloat(), 5f, false, "rpm (x1000)"
    )
    LinearGauge(Modifier.offset(50.dp, 150.dp),
        fuel.toFloat(), 1f, "images/fuel.png", Pair('E', 'F')
    )
    LinearGauge(Modifier.offset(125.dp, 150.dp),
        temp.toFloat(), 100f, "images/temp.png", Pair('C', 'H')
    )
    Image(
        painter = painterResource("images/logo.png"),
        contentDescription = "",
        modifier = Modifier
            .size(200.dp)
            .offset(540.dp, 50.dp)
    )
}