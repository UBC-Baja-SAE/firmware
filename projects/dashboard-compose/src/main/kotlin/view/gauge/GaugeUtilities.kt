package org.baja.dashboard.view.gauge

fun boundedPercentage(current: Float, max: Float, min: Float = 0f) =
    if (current < 0f)
        0f
    else if (current > max)
        1f
    else
        current / max