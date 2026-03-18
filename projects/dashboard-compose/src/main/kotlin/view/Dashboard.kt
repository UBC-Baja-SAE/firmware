package view

import androidx.compose.animation.core.*
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.delay
import org.baja.dashboard.view.gauge.LinearGauge
import org.baja.dashboard.view.gauge.RadialGauge
import org.baja.dashboard.viewmodel.DataViewModel
import org.baja.dashboard.view.resources.BAJA_PURPLE
import java.net.NetworkInterface
import java.time.LocalTime
import java.time.format.DateTimeFormatter

@Composable
fun Dashboard() {
    val speed by DataViewModel.getSpeed().collectAsState()
    val rpm by DataViewModel.getRPM().collectAsState()
    val gpsMode by DataViewModel.getGpsMode().collectAsState()
    
    var networkSSID by remember { mutableStateOf("Disconnected") }
    var localIp by remember { mutableStateOf("0.0.0.0") }
    var currentTime by remember { mutableStateOf(LocalTime.now().format(DateTimeFormatter.ofPattern("HH:mm:ss"))) }

    var targetFuel by remember { mutableStateOf(1.0f) }
    var targetTemp by remember { mutableStateOf(45.0f) }

    LaunchedEffect(Unit) {
        while (true) {  
            currentTime = LocalTime.now().format(DateTimeFormatter.ofPattern("HH:mm:ss"))
            targetFuel = if (targetFuel > 0.05f) targetFuel - 0.02f else 1.0f
            targetTemp = (40..60).random().toFloat()
            delay(1000)
        }
    }

    LaunchedEffect(Unit) {
        while (true) {
            try {
                val ip = NetworkInterface.getNetworkInterfaces().toList()
                    .flatMap { it.inetAddresses.toList() }
                    .firstOrNull { !it.isLoopbackAddress && it.hostAddress.contains(".") }
                    ?.hostAddress ?: "No IP"
                localIp = ip
                val process = Runtime.getRuntime().exec("iwgetid -r")
                networkSSID = process.inputStream.bufferedReader().readText().trim().ifEmpty { "No WiFi" }
            } catch (e: Exception) {
                localIp = "Error"
            }
            delay(5000)
        }
    }

    val animatedSpeed by animateFloatAsState(targetValue = speed.toFloat(), animationSpec = tween(500, easing = LinearOutSlowInEasing))
    val animatedRpm by animateFloatAsState(targetValue = (rpm / 1000.0).toFloat(), animationSpec = tween(200, easing = LinearEasing))
    val animatedFuel by animateFloatAsState(targetValue = targetFuel, animationSpec = tween(1500))
    val animatedTemp by animateFloatAsState(targetValue = targetTemp, animationSpec = spring())

    Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
        
        Column(modifier = Modifier.align(Alignment.TopStart).padding(top = 10.dp, start = 20.dp)) {
            Text(text = if (gpsMode >= 2) "🛰️ LOCKED" else "🛰️ SEARCH", color = if (gpsMode >= 2) Color.Green else Color.Yellow, fontSize = 16.sp)
            Spacer(modifier = Modifier.height(5.dp))
            Text(text = "📶 $networkSSID", color = BAJA_PURPLE, fontSize = 14.sp)
            Text(text = "IP: $localIp", color = BAJA_PURPLE, fontSize = 12.sp, fontFamily = FontFamily.Monospace)
        }

        // --- All main components lowered to Y = 120 ---
        
        LinearGauge(Modifier.offset(x = 30.dp, y = 140.dp), animatedFuel, 1f, "images/fuel.png", Pair('E', 'F'))
        LinearGauge(Modifier.offset(x = 100.dp, y = 140.dp), animatedTemp, 100f, "images/temp.png", Pair('C', 'H'))

        RadialGauge(
            modifier = Modifier.offset(x = 180.dp, y = 120.dp),
            currentValue = animatedSpeed,
            maxValue = 60f,
            showReading = true,
            label = "km/h"
        )
        
        Image(
            painter = painterResource("images/logo.png"),
            contentDescription = null,
            modifier = Modifier.size(160.dp).offset(x = 560.dp, y = 120.dp)
        )

        RadialGauge(
            modifier = Modifier.offset(x = 740.dp, y = 120.dp),
            currentValue = animatedRpm,
            maxValue = 6f,
            showReading = true,
            label = "RPM",
            textScale = 1000
        )

        Text(
            text = currentTime,
            color = Color.White,
            fontSize = 32.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.align(Alignment.TopCenter).padding(bottom = 5.dp)
        )
    }
}