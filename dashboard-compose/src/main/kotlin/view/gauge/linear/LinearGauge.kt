package org.baja.dashboard.view.gauge.linear

import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import org.baja.dashboard.view.resources.BAJA_PURPLE

/**
 * The height and width of the bar that fills up the gauge.
 */
private const val BAR_HEIGHT = 200f
private const val BAR_WIDTH = 25f

/**
 * The colour gradient of the infill for the gauge. The float values are bound
 * from `0f` and `1f`, representing the start and end. The float
 * values represent the point along the bar where its associated colour begins,
 * where the float value is the percentage along the bar, from top to bottom.
 */
private val GRADIENT = arrayOf(
    0.0f to Color.Red,
    0.15f to BAJA_PURPLE,
    0.4f to BAJA_PURPLE,
    0.9f to Color.White,
    1.0f to Color.LightGray
)

/**
 * The time to animate from one height to another.
 */
private const val BAR_CHANGE_INTERVAL = 1000 / 30

/**
 * The width of the outside border, and the offset required to move the border
 * accordingly.
 */
private const val BORDER_WIDTH = 3f
private const val OFFSET = BORDER_WIDTH / 2

/**
 * The length of the center tick and the outer ticks.
 */
private const val TICK_LENGTH_SMALL = BAR_WIDTH * 0.6f + BORDER_WIDTH
private const val TICK_LENGTH_LARGE = BAR_WIDTH + BORDER_WIDTH

/**
 * The size of the icons next to the center of the gauge.
 */
private const val IMAGE_SIZE = 40

/**
 * A gauge that fills linearly vertically. The gauge fills from bottom to top,
 * and it is fully filled when `currentValue` is equal to `maxValue`.
 * @param modifier  the modifier for the `LinearGauge`'s bounding box. Use this
 *                  parameter to modify the position of the gauge.
 * @param currentValue  the current value to display on the gauge.
 * @param maxValue  the maximum value that can be displayed on the gauge.
 * @param imagePath the filepath for the icon to display, relative to the
 *                  `src/main/resources` directory.
 */
@Composable
fun LinearGauge(
    modifier: Modifier,
    currentValue: Float,
    maxValue: Float,
    imagePath: String
) {
    val percentage = if (currentValue >= 0f) currentValue / maxValue else 0f
    val targetHeight by animateFloatAsState(
        targetValue = BAR_HEIGHT * percentage,
        animationSpec = tween(
            durationMillis = BAR_CHANGE_INTERVAL,
            easing = LinearEasing
        )
    )

    Box(
        modifier = modifier
            .height(BAR_HEIGHT.dp)
            .width(BAR_WIDTH.dp),
        contentAlignment = Alignment.Center
    ) {
        Canvas(modifier = Modifier.matchParentSize()) {
            val left = -OFFSET
            val right = TICK_LENGTH_LARGE
            val top = left
            val bottom = OFFSET + BAR_HEIGHT
            val center = BAR_HEIGHT / 2
            drawLine(left, top, left, bottom)
            drawLine(left, top, right, top)
            drawLine(left, bottom, right, bottom)
            drawLine(left, center, TICK_LENGTH_SMALL, center)
            fillGauge(targetHeight)
        }
        DrawIcon(imagePath)
    }
}

/**
 * Draw the icon beside the gauge.
 * @param imagePath the filepath for the icon to display, relative to the
 *                  `src/main/resources` directory.
 */
@Composable
fun DrawIcon(imagePath: String) {
    Image(
        painter = painterResource(imagePath),
        contentDescription = "",
        modifier = Modifier
            .size(IMAGE_SIZE.dp)
            .offset(
                (TICK_LENGTH_LARGE / 2 + IMAGE_SIZE / 2).dp,
                (BORDER_WIDTH / 2).dp
            )
    )
}

/**
 * Draw the fill on the gauge.
 * @param targetHeight  the height that the gauge should fill to, measured in
 *                      `dp`.
 */
fun DrawScope.fillGauge(targetHeight: Float) {
    val gradientBrush = Brush.linearGradient(
        colorStops = GRADIENT
    )
    drawLine(
        brush = gradientBrush,
        start = Offset(BAR_WIDTH / 2, BAR_HEIGHT),
        end = Offset(BAR_WIDTH / 2, BAR_HEIGHT - targetHeight),
        strokeWidth = BAR_WIDTH
    )
}

/**
 * Draw a line from point `(x1, y1)` to point `(x2, y2)`.
 */
fun DrawScope.drawLine(x1: Float, y1: Float, x2: Float, y2: Float) {
    drawLine(
        color = Color.White,
        strokeWidth = BORDER_WIDTH,
        start = Offset(x1, y1),
        end = Offset(x2, y2),
        cap = StrokeCap.Square
    )
}