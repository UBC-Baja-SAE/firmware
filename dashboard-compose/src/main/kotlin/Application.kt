package org.baja.dashboard

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.size
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.WindowPlacement
import androidx.compose.ui.window.WindowState
import androidx.compose.ui.window.application
import view.Dashboard

/**
 * Entry point for the dashboard application.
 */
fun main() = application {
    Window(
        onCloseRequest = ::exitApplication,
        // full screen
        state = WindowState(
            placement = WindowPlacement.Maximized
        ),
        // removes top bar
        undecorated = true
    ) {
        Box(
            modifier = Modifier
                .background(Color.Black)
//                .fillMaxSize()
                .size(1280.dp, 400.dp)
        ) {
            Dashboard()
        }
    }
}