package org.baja.dashboard.view.gauge

import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.*
import androidx.compose.material.Text
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
import androidx.compose.ui.unit.sp
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
private val LINEAR_GRADIENT = arrayOf(
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
private const val LINEAR_TICK_LENGTH_SMALL = BAR_WIDTH * 0.8f
private const val LINEAR_TICK_LENGTH_LARGE = BAR_WIDTH

/**
 * The size of the icons next to the center of the gauge.
 */
private const val IMAGE_SIZE = 30

/**
 * The font size of the symbols next to the tick marks.
 */
private const val SYMBOL_FONT_SIZE = 20

/**
 * A gauge that fills linearly vertically. The gauge fills from bottom to top,
 * and it is fully filled when `currentValue` is equal to `maxValue`.
 * @param modifier  the modifier for the `LinearGauge`'s bounding box. Use this
 *  parameter to modify the position of the gauge.
 * @param currentValue  the current value to display on the gauge.
 * @param maxValue  the maximum value that can be displayed on the gauge.
 * @param imagePath the filepath for the icon to display, relative to the
 *  src/main/resources` directory.
 * @param symbols   the symbols that indicate the meaning of the gauge when it
 *  is full or empty. The elements will be drawn with the `first` and `second`
 *  at the bottom and top of the gauge respectively.
 */
@Composable
fun LinearGauge(
    modifier: Modifier,
    currentValue: Float,
    maxValue: Float,
    imagePath: String,
    symbols: Pair<Char, Char>
) {
    val percentage = boundedPercentage(currentValue, maxValue)
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
        val left = -OFFSET
        val right = LINEAR_TICK_LENGTH_LARGE
        val top = left
        val bottom = BAR_HEIGHT + OFFSET
        val center = BAR_HEIGHT / 2

        Canvas(
            modifier = Modifier
                .height(BAR_HEIGHT.dp)
                .width(BAR_WIDTH.dp)
        ) {
            drawLine(left, top, left, bottom)
            drawLine(left, top, right, top)
            drawLine(left, bottom, right, bottom)
            drawLine(left, center, LINEAR_TICK_LENGTH_SMALL, center)

            fillLinearGauge(targetHeight)
        }
        Icon(imagePath)
        Symbol(symbols.first, bottom)
        Symbol(symbols.second, top)
    }
}

/**
 * Draw the icon beside the gauge.
 * @param imagePath the filepath for the icon to display, relative to the
 *                  `src/main/resources` directory.
 */
@Composable
private fun Icon(imagePath: String) {
    Image(
        painter = painterResource(imagePath),
        contentDescription = "",
        modifier = Modifier
            .size(IMAGE_SIZE.dp)
            .offset(
                x = (LINEAR_TICK_LENGTH_LARGE / 2 + IMAGE_SIZE / 2).dp
            )
    )
}

/**
 * Draws the [symbol] at the end of the tick mark at [height].
 * @param symbol    the symbol to be displayed.
 * @param height    the height to draw the symbol starting at.
 */
@Composable
private fun Symbol(symbol: Char, height: Float) {
    Text(
        text = "$symbol",
        fontSize = SYMBOL_FONT_SIZE.sp,
        color = Color.White,
        modifier = Modifier.offset(
            x = (LINEAR_TICK_LENGTH_LARGE / 2 + SYMBOL_FONT_SIZE / 2).dp,
            y = (height - BAR_HEIGHT / 2).dp
        )
    )
}

/**
 * Draw the fill on the gauge.
 * @param targetHeight  the height that the gauge should fill to, measured in
 *                      `dp`.
 */
private fun DrawScope.fillLinearGauge(targetHeight: Float) {
    val gradientBrush = Brush.linearGradient(
        colorStops = LINEAR_GRADIENT
    )
    val x = (BAR_WIDTH / 2).dp.toPx()
    drawLine(
        brush = gradientBrush,
        start = Offset(x, BAR_HEIGHT.dp.toPx()),
        end = Offset(x, (BAR_HEIGHT - targetHeight).dp.toPx()),
        strokeWidth = BAR_WIDTH.dp.toPx()
    )
}

/**
 * Draw a line from point `(x1, y1)` to point `(x2, y2)`.
 */
private fun DrawScope.drawLine(x1: Float, y1: Float, x2: Float, y2: Float) {
    drawLine(
        color = Color.White,
        strokeWidth = BORDER_WIDTH,
        start = Offset(x1.dp.toPx(), y1.dp.toPx()),
        end = Offset(x2.dp.toPx(), y2.dp.toPx()),
        cap = StrokeCap.Square
    )
}