package org.baja.dashboard.view.gauge

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
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import org.baja.dashboard.view.resources.BAJA_PURPLE
import kotlin.math.cos
import kotlin.math.sin

private const val START_ANGLE = 165f
private const val END_ANGLE = 15f
private const val ANGLE_RANGE = 360f - (START_ANGLE - END_ANGLE)
private const val GAUGE_SIZE = 360 

private val RADIAL_GRADIENT = arrayOf(
    0.0f to BAJA_PURPLE,
    0.05f to Color.Red,
    0.3f to Color.LightGray,
    0.4f to Color.White,
    0.8f to BAJA_PURPLE
)

private const val ANGLE_CHANGE_INTERVAL = 1000 / 30
private const val READING_FONT_SIZE = 90

/**
 * KEPT AT 45: This maintains the low position relative to the center 
 * that you preferred.
 */
private const val READING_VERTICAL_OFFSET = 45

private const val LABEL_FONT_SIZE = 26
private const val LABEL_VERTICAL_OFFSET = READING_VERTICAL_OFFSET + 55

private const val MEASUREMENT_FONT_SIZE = 22
private const val MEASUREMENT_OFFSET = 20
private const val BORDER_RADIUS = 5
private const val FILL_RADIUS = 45 
private const val TOTAL_RADIUS = FILL_RADIUS + BORDER_RADIUS
private const val TICK_WIDTH_LARGE = 4
private const val TICK_WIDTH_SMALL = 2
private const val RADIAL_TICK_LENGTH_LARGE = 45 + BORDER_RADIUS
private const val RADIAL_TICK_LENGTH_SMALL = 35 + BORDER_RADIUS
private const val TICK_COUNT = 7

@Composable
fun RadialGauge(
    modifier: Modifier,
    currentValue: Float,
    maxValue: Float,
    showReading: Boolean = false,
    label: String = "",
    textScale: Int = 1
) {
    val percentage = (currentValue / maxValue).coerceIn(0f, 1f)
    val targetAngle by animateFloatAsState(
        targetValue = ANGLE_RANGE * percentage,
        animationSpec = tween(durationMillis = ANGLE_CHANGE_INTERVAL, easing = LinearEasing)
    )

    Box(
        modifier = modifier.size(GAUGE_SIZE.dp),
        contentAlignment = Alignment.Center 
    ) {
        val angleIncrements = ANGLE_RANGE / (TICK_COUNT - 1)

        Canvas(modifier = Modifier.size(GAUGE_SIZE.dp)) {
            drawBorder()
            drawTicks(TICK_COUNT, RADIAL_TICK_LENGTH_LARGE, TICK_WIDTH_LARGE, START_ANGLE, angleIncrements)
            drawTicks(TICK_COUNT - 1, RADIAL_TICK_LENGTH_SMALL, TICK_WIDTH_SMALL, START_ANGLE + angleIncrements * 0.5f, angleIncrements)
            fillRadialGauge(targetAngle)
        }
        
        DrawMeasurements(angleIncrements, maxValue, textScale)
        
        if (showReading) {
            Text(
                text = "${(currentValue * textScale).toInt()}",
                fontSize = READING_FONT_SIZE.sp,
                fontWeight = FontWeight.Bold,
                color = Color.White,
                textAlign = TextAlign.Center,
                modifier = Modifier
                    .align(Alignment.Center)
                    .offset(y = READING_VERTICAL_OFFSET.dp)
            )
        }
        Text(
            text = label,
            fontSize = LABEL_FONT_SIZE.sp,
            color = Color.LightGray,
            textAlign = TextAlign.Center,
            modifier = Modifier
                .align(Alignment.Center)
                .offset(y = LABEL_VERTICAL_OFFSET.dp)
        )
    }
}

private fun DrawScope.drawBorder() {
    drawArc(
        color = Color.White,
        startAngle = START_ANGLE,
        sweepAngle = ANGLE_RANGE,
        useCenter = false,
        style = Stroke(width = BORDER_RADIUS.dp.toPx(), cap = StrokeCap.Butt),
        size = Size(size.width, size.height)
    )
}

private fun DrawScope.drawTicks(tickCount: Int, tickLength: Int, tickWidth: Int, startAngle: Float, angleIncrements: Float) {
    val center = GAUGE_SIZE / 2f
    for (i in 0 until tickCount) {
        val degrees = startAngle + angleIncrements * i
        val radians = Math.toRadians(degrees.toDouble())
        val ratioX = cos(radians).toFloat()
        val ratioY = sin(radians).toFloat()
        val endRadius = center + BORDER_RADIUS / 2f
        val startRadius = endRadius - tickLength
        val startX = center + startRadius * ratioX
        val startY = center + startRadius * ratioY
        val endX = center + endRadius * ratioX
        val endY = center + endRadius * ratioY
        drawLine(
            color = Color.White,
            start = Offset(startX.dp.toPx(), startY.dp.toPx()),
            end = Offset(endX.dp.toPx(), endY.dp.toPx()),
            strokeWidth = tickWidth.dp.toPx()
        )
    }
}

private fun DrawScope.fillRadialGauge(targetAngle: Float) {
    val gradientBrush = Brush.sweepGradient(colorStops = RADIAL_GRADIENT, center = Offset(center.x, center.y))
    val arcSize = GAUGE_SIZE - TOTAL_RADIUS
    drawArc(
        brush = gradientBrush,
        startAngle = START_ANGLE,
        sweepAngle = targetAngle,
        useCenter = false,
        style = Stroke(width = FILL_RADIUS.dp.toPx(), cap = StrokeCap.Butt),
        size = Size(arcSize.dp.toPx(), arcSize.dp.toPx()),
        topLeft = Offset((TOTAL_RADIUS / 2f).dp.toPx(), (TOTAL_RADIUS / 2f).dp.toPx())
    )
}

@Composable
private fun DrawMeasurements(angleIncrements: Float, maxValue: Float, textScale: Int) {
    val center = GAUGE_SIZE / 2f
    for (i in 0 until TICK_COUNT) {
        val degrees = START_ANGLE + angleIncrements * i
        val radians = Math.toRadians(degrees.toDouble())
        val ratioX = cos(radians).toFloat()
        val ratioY = sin(radians).toFloat()
        val distance = center - MEASUREMENT_OFFSET - RADIAL_TICK_LENGTH_LARGE
        val x = distance * ratioX
        val y = distance * ratioY
        val value = (maxValue * i / (TICK_COUNT - 1) * textScale).toInt()
        
        Box(modifier = Modifier.size(GAUGE_SIZE.dp), contentAlignment = Alignment.Center) {
            Text(
                text = "$value",
                color = Color.White,
                fontSize = MEASUREMENT_FONT_SIZE.sp,
                textAlign = TextAlign.Center,
                modifier = Modifier.offset(x.dp, y.dp)
            )
        }
    }
}