package org.baja.dashboard.view.gauge.radial

import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import org.baja.dashboard.view.resources.BAJA_PURPLE

const val START_ANGLE = 165f
const val END_ANGLE = 15f
const val ANGLE_RANGE = 360f - (START_ANGLE - END_ANGLE)

const val SIZE =  300

const val STROKE_WIDTH = 50

val GRADIENT = arrayOf(
    0.0f to BAJA_PURPLE,
    0.05f to Color.Red,
    0.3f to Color.LightGray,
    0.4f to Color.White,
    0.8f to BAJA_PURPLE
)

const val ANGLE_CHANGE_INTERVAL = 1000 / 30

@Composable
fun RadialGauge(
    modifier: Modifier,
    currentValue: Float,
    maxValue: Float
) {
    val currentPercentage = currentValue / maxValue
    val targetAngle by animateFloatAsState(
        targetValue = ANGLE_RANGE * currentPercentage,
        animationSpec = tween(
            durationMillis = ANGLE_CHANGE_INTERVAL,
            easing = LinearEasing
        )
    )

    Canvas(modifier = modifier.size(SIZE.dp)) {
        val gradientBrush = Brush.sweepGradient(
            colorStops = GRADIENT,
            center = Offset(center.x, center.y)
        )

        drawArc(
            brush = gradientBrush,
            startAngle = START_ANGLE,
            sweepAngle = targetAngle,
            useCenter = false,
            style = Stroke(
                width = STROKE_WIDTH.toFloat(),
                cap = StrokeCap.Square
            ),
            size = size
        )
    }
}