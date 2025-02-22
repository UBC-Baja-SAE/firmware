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
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import org.baja.dashboard.view.resources.BAJA_PURPLE
import kotlin.math.cos
import kotlin.math.sin

/**
 * The angles that represent the gauge at zero, max, and the total range,
 * respectively. The UI components that utilise these values represent angles as
 * degrees clockwise from the 3 o'clock direction.
 */
private const val START_ANGLE = 165f
private const val END_ANGLE = 15f
private const val ANGLE_RANGE = 360f - (START_ANGLE - END_ANGLE)

/**
 * The width/length of the gauge. This unit is cast to `dp`, or
 * [density-independent pixels](https://developer.android.com/training/multiscreen/screendensities).
 * The outer radius of the gauge will change accordingly, and the inner radius
 * and inner tick value positions will match the position of the outer radius.
 */
private const val SIZE = 400

/**
 * The colour gradient of the infill for the gauge. The float values are bound
 * from `0f` and `1f`, representing the start and end at 3 o'clock. The float
 * values represent the point along the arc where its associated colour begins,
 * where the float value is the percentage along the arc, clockwise.
 * There is a
 * hard change between colours at `0f` and `1f`.
 */
private val GRADIENT = arrayOf(
    0.0f to BAJA_PURPLE,
    0.05f to Color.Red,
    0.3f to Color.LightGray,
    0.4f to Color.White,
    0.8f to BAJA_PURPLE
)

/**
 * The time to animate from one angle to another.
 */
private const val ANGLE_CHANGE_INTERVAL = 1000 / 30

/**
 * The font size for the main value reading in `sp`, and its vertical offset.
 */
private const val READING_FONT_SIZE = 135
private const val READING_VERTICAL_OFFSET = -20

/**
 * The font size for the reading's label in `sp`, and its vertical offset
 */
private const val LABEL_FONT_SIZE = 30
private const val LABEL_VERTICAL_OFFSET = READING_VERTICAL_OFFSET + 85

/**
 * The font size for the measurements just inside the large tick marks in `sp`,
 * and the radial offset from the start of its tick mark.
 */
private const val MEASUREMENT_FONT_SIZE = 28
private const val MEASUREMENT_OFFSET = 24

/**
 * The total width of the primary outer border, the total width of the fill
 * gauge, and the sum of both, in `dp`.
 */
private const val BORDER_RADIUS = 5
private const val FILL_RADIUS = 50
private const val TOTAL_RADIUS = FILL_RADIUS + BORDER_RADIUS

/**
 * The total width and lengths of large and small tick marks, in `dp`.
 */
private const val TICK_WIDTH_LARGE = 5
private const val TICK_WIDTH_SMALL = 3
private const val TICK_LENGTH_LARGE = 50 + BORDER_RADIUS
private const val TICK_LENGTH_SMALL = 40 + BORDER_RADIUS

/**
 * The number of large ticks around the gauge. There are always exactly
 * `TICK_COUNT - 1` small ticks, with one between each large tick.
 */
private const val TICK_COUNT = 7

/**
 * A gauge that fills radially. The gauge fills clock-wise, and it is fully
 * filled when `currentValue` is equal to `maxValue`. Visually, the reading
 * on the gauge is bounded between `0f` and `maxValue`.
 *
 * @param modifier  the modifier for the `RadialGauge`'s bounding box. Use this
 *                  parameter to modify the position of the gauge.
 * @param currentValue  the current value to display on the gauge.
 * @param maxValue  the maximum value that can be displayed on the gauge.
 * @param showReading   whether to show the central numerical reading.
 * @param label the text to display underneath the central numerical reading.
 */
@Composable
fun RadialGauge(
    modifier: Modifier,
    currentValue: Float,
    maxValue: Float,
    showReading: Boolean = false,
    label: String = ""
) {
    val percentage = if (currentValue >= 0f) currentValue / maxValue else 0f
    val targetAngle by animateFloatAsState(
        targetValue = ANGLE_RANGE * percentage,
        animationSpec = tween(
            durationMillis = ANGLE_CHANGE_INTERVAL,
            easing = LinearEasing
        )
    )

    Box(
        modifier = modifier.size(SIZE.dp),
        contentAlignment = Alignment.Center
    ) {
        val angleIncrements = ANGLE_RANGE / (TICK_COUNT - 1)

        Canvas(modifier = Modifier.matchParentSize()) {
            drawBorder()
            drawTicks(
                tickCount = TICK_COUNT,
                tickLength = TICK_LENGTH_LARGE,
                tickWidth = TICK_WIDTH_LARGE,
                startAngle = START_ANGLE,
                angleIncrements = angleIncrements
            )
            drawTicks(
                tickCount = TICK_COUNT - 1,
                tickLength = TICK_LENGTH_SMALL,
                tickWidth = TICK_WIDTH_SMALL,
                startAngle = START_ANGLE + angleIncrements * 0.5f,
                angleIncrements = angleIncrements
            )
            fillGauge(targetAngle)
        }
        DrawMeasurements(
            angleIncrements = angleIncrements,
            maxValue = maxValue
        )
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

/**
 * Draw a circular border.
 */
fun DrawScope.drawBorder() {
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
}

/**
 * Draw ticks spaced evenly throughout a circular arc.
 *
 * @param tickCount     the number of ticks to draw.
 * @param tickLength    the length of the ticks to draw.
 * @param tickWidth     the width of the ticks to draw.
 * @param startAngle    the angle where the first tick should be drawn in
 *  degrees measured clockwise from the 3 o'clock position.
 * @param angleIncrements   the angle in degrees to space the ticks by.
 */
fun DrawScope.drawTicks(
    tickCount: Int,
    tickLength: Int,
    tickWidth: Int,
    startAngle: Float,
    angleIncrements: Float
) {
    for (i in 0 until tickCount) {
        val degrees = startAngle + angleIncrements * i
        val radians = Math.toRadians(degrees.toDouble())

        val ratioX = cos(radians).toFloat()
        val ratioY = sin(radians).toFloat()

        val endRadius = SIZE / 2 + BORDER_RADIUS / 2
        val startRadius = endRadius - tickLength

        val center = SIZE / 2

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

/**
 * Draw the fill of the gauge.
 * @param targetAngle   the angle that the gauge should fill to, measured in
 *                      degrees from the start angle of the gauge.
 */
fun DrawScope.fillGauge(targetAngle: Float) {
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

/**
 * Element that displays the incremental values corresponding to the large tick
 * marks in the gauge.
 * @param angleIncrements   the angle in degrees that the ticks are spaced by.
 * @param maxValue  the maximum value that can be measured by the gauge. The
 *                  last measurement value corresponds to this value.
 */
@Composable
fun DrawMeasurements(
    angleIncrements: Float,
    maxValue: Float
) {
    for (i in 0 until TICK_COUNT) {
        val degrees = START_ANGLE + angleIncrements * i
        val radians = Math.toRadians(degrees.toDouble())

        val ratioX = cos(radians).toFloat()
        val ratioY = sin(radians).toFloat()

        val distance = SIZE / 2 - MEASUREMENT_OFFSET - TICK_LENGTH_LARGE

        val x = distance * ratioX
        val y = distance * ratioY

        val value = (maxValue * i / (TICK_COUNT - 1)).toInt()
        Text(
            text = "$value",
            color = Color.White,
            fontSize = MEASUREMENT_FONT_SIZE.sp,
            textAlign = TextAlign.Center,
            modifier = Modifier
                .offset(x.dp, y.dp)
        )
    }
}
