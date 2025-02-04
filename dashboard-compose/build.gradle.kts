import org.jetbrains.compose.desktop.application.dsl.TargetFormat

repositories {
    gradlePluginPortal()
    mavenCentral()
    google()
}

plugins {
    kotlin("jvm")
    kotlin("plugin.compose")
    id("org.jetbrains.compose")
}

group = "org.baja.dashboard"
version = "1.0"

dependencies {
    implementation(compose.desktop.currentOs)
}

compose.desktop {
    application {
        mainClass = "org.baja.dashboard.ApplicationKt"

        nativeDistributions {
            targetFormats(TargetFormat.Msi, TargetFormat.Deb)
            packageName = "Dashboard"
            packageVersion = "1.0.0"
            description = "Compose UI Dashboard App"
        }
    }
}