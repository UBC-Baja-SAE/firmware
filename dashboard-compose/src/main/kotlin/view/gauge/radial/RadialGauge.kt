package org.baja.dashboard.view.gauge.radial

import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.size
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import org.baja.dashboard.view.resources.BAJA_PURPLE
import kotlin.math.cos
import kotlin.math.sin

private const val START_ANGLE = 165f
private const val END_ANGLE = 15f
private const val ANGLE_RANGE = 360f - (START_ANGLE - END_ANGLE)

private const val SIZE = 400

private val GRADIENT = arrayOf(
    0.0f to BAJA_PURPLE,
    0.05f to Color.Red,
    0.3f to Color.LightGray,
    0.4f to Color.White,
    0.8f to BAJA_PURPLE
)

private const val ANGLE_CHANGE_INTERVAL = 1000 / 30

private const val READING_FONT_SIZE = 150
private const val READING_VERTICAL_OFFSET = -40

private const val LABEL_FONT_SIZE = 30
private const val LABEL_VERTICAL_OFFSET = READING_VERTICAL_OFFSET + 85

private const val BORDER_RADIUS = 5
private const val FILL_RADIUS = 50
private const val TOTAL_RADIUS = FILL_RADIUS + BORDER_RADIUS

private const val TICK_WIDTH_LARGE = 5
private const val TICK_WIDTH_SMALL = 3
private const val TICK_LENGTH_LARGE = 50 + BORDER_RADIUS
private const val TICK_LENGTH_SMALL = 40 + BORDER_RADIUS

private const val TICK_COUNT = 7
private const val SMALL_TICK_COUNT = TICK_COUNT - 1

@Composable
fun RadialGauge(
    modifier: Modifier,
    currentValue: Float,
    maxValue: Float,
    showReading: Boolean = false,
    label: String = ""
) {
    val currentPercentage = currentValue / maxValue
    val targetAngle by animateFloatAsState(
        targetValue = ANGLE_RANGE * currentPercentage,
        animationSpec = tween(
            durationMillis = ANGLE_CHANGE_INTERVAL,
            easing = LinearEasing
        )
    )

    Box(
        modifier = modifier.size(SIZE.dp),
        contentAlignment = Alignment.Center
    ) {
        Canvas(modifier = Modifier.matchParentSize()) {
            drawArc(
                color = Color.White,
                startAngle = START_ANGLE,
                sweepAngle = ANGLE_RANGE,
                useCenter = false,
                style = Stroke(
                    width = BORDER_RADIUS.dp.toPx(),
                    cap = StrokeCap.Butt
                ),
                size = size
            )

            val gradientBrush = Brush.sweepGradient(
                colorStops = GRADIENT,
                center = Offset(center.x, center.y)
            )

            val tickAngleIncrement = ANGLE_RANGE / (TICK_COUNT - 1)
            for (i in 0 until TICK_COUNT) {
                val degrees = (START_ANGLE + i * tickAngleIncrement).toDouble()
                val radians = Math.toRadians(degrees)

                val ratioX = cos(radians).toFloat()
                val ratioY = sin(radians).toFloat()

                val endRadius = SIZE / 2 + BORDER_RADIUS / 2
                val startRadius = endRadius - TICK_LENGTH_LARGE

                val startX = center.x + startRadius * ratioX
                val startY = center.y + startRadius * ratioY

                val endX = center.x + endRadius * ratioX
                val endY = center.y + endRadius * ratioY

                drawLine(
                    color = Color.White,
                    start = Offset(startX, startY),
                    end = Offset(endX, endY),
                    strokeWidth = TICK_WIDTH_LARGE.dp.toPx()
                )
            }

            for (i in 0 until SMALL_TICK_COUNT) {
                val degrees = START_ANGLE + tickAngleIncrement * (0.5 + i)
                val radians = Math.toRadians(degrees)

                val ratioX = cos(radians).toFloat()
                val ratioY = sin(radians).toFloat()

                val endRadius = SIZE / 2 + BORDER_RADIUS / 2
                val startRadius = endRadius - TICK_LENGTH_SMALL

                val startX = center.x + startRadius * ratioX
                val startY = center.y + startRadius * ratioY

                val endX = center.x + endRadius * ratioX
                val endY = center.y + endRadius * ratioY

                drawLine(
                    color = Color.White,
                    start = Offset(startX, startY),
                    end = Offset(endX, endY),
                    strokeWidth = TICK_WIDTH_SMALL.dp.toPx()
                )
            }

            drawArc(
                brush = gradientBrush,
                startAngle = START_ANGLE,
                sweepAngle = targetAngle,
                useCenter = false,
                style = Stroke(
                    width = FILL_RADIUS.dp.toPx(),
                    cap = StrokeCap.Butt
                ),
                size = Size(
                    size.width - TOTAL_RADIUS.dp.toPx(),
                    size.height - TOTAL_RADIUS.dp.toPx()
                ),
                topLeft = Offset(
                    (TOTAL_RADIUS / 2).dp.toPx(), (TOTAL_RADIUS / 2).dp.toPx()
                )
            )
        }
        if (showReading) {
            Text(
                text = "${currentValue.toInt()}",
                fontSize = READING_FONT_SIZE.sp,
                fontWeight = FontWeight.Bold,
                color = Color.White,
                modifier = Modifier.offset(
                    y = READING_VERTICAL_OFFSET.dp
                )
            )
        }
        Text(
            text = label,
            fontSize = LABEL_FONT_SIZE.sp,
            color = Color.LightGray,
            modifier = Modifier.offset(
                y = LABEL_VERTICAL_OFFSET.dp
            )
        )
    }
}