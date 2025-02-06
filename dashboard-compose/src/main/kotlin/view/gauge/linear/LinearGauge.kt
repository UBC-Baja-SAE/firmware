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
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import org.baja.dashboard.view.resources.BAJA_PURPLE

private const val BAR_HEIGHT = 200f
private const val BAR_WIDTH = 25f

private val GRADIENT = arrayOf(
    0.0f to Color.Red,
    0.15f to BAJA_PURPLE,
    0.4f to BAJA_PURPLE,
    0.9f to Color.White,
    1.0f to Color.LightGray
)

private const val BAR_CHANGE_INTERVAL = 1000 / 30

private const val BORDER_WIDTH = 3f

private const val TICK_WIDTH = 2f
private const val TICK_LENGTH_LARGE = BAR_WIDTH + BORDER_WIDTH
private const val TICK_LENGTH_SMALL = BAR_WIDTH * 0.6f + BORDER_WIDTH

private const val TOP_CENTER_Y = BAR_HEIGHT + BORDER_WIDTH / 2

private const val IMAGE_SIZE = 40


@Composable
fun LinearGauge(
    modifier: Modifier,
    currentValue: Float,
    maxValue: Float,
    imagePath: String
) {
    val currentPercentage = currentValue / maxValue
    val targetHeight by animateFloatAsState(
        targetValue = BAR_HEIGHT * currentPercentage,
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
            drawLine(
                color = Color.White,
                strokeWidth = BORDER_WIDTH,
                start = Offset(0f, BORDER_WIDTH / 2),
                end = Offset(0f, BORDER_WIDTH / 2 + BAR_HEIGHT)
            )
            drawLine(
                color = Color.White,
                strokeWidth = BORDER_WIDTH,
                start = Offset(0f, BAR_HEIGHT + BORDER_WIDTH),
                end = Offset(TICK_LENGTH_LARGE, BAR_HEIGHT + BORDER_WIDTH),
                cap = StrokeCap.Square
            )
            drawLine(
                color = Color.White,
                strokeWidth = TICK_WIDTH,
                start = Offset(0f, size.height / 2),
                end = Offset(TICK_LENGTH_SMALL, size.height / 2)
            )
            drawLine(
                color = Color.White,
                strokeWidth = BORDER_WIDTH,
                start = Offset(0f, 0f),
                end = Offset(TICK_LENGTH_LARGE, 0f),
                cap = StrokeCap.Square
            )

            val gradientBrush = Brush.linearGradient(
                colorStops = GRADIENT
            )
            drawLine(
                brush = gradientBrush,
                start = Offset(
                    BORDER_WIDTH / 2 + BAR_WIDTH / 2,
                    BORDER_WIDTH / 2 + BAR_HEIGHT
                ),
                end = Offset(
                    BORDER_WIDTH / 2 + BAR_WIDTH / 2,
                    BORDER_WIDTH / 2 + BAR_HEIGHT - targetHeight
                ),
                strokeWidth = BAR_WIDTH
            )
        }
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
}