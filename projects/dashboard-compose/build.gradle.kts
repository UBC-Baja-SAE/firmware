import org.jetbrains.compose.desktop.application.dsl.TargetFormat
import org.jetbrains.kotlin.gradle.dsl.JvmTarget

repositories {
    gradlePluginPortal()
    mavenCentral()
    google()
}

plugins {
    kotlin("jvm") version "2.1.20"
    id("org.jetbrains.compose") version "1.6.10"
    id("org.jetbrains.kotlin.plugin.compose") version "2.1.20"
}

group = "org.baja.dashboard"
version = "1.0"

dependencies {
    implementation(compose.desktop.currentOs)
}

kotlin {
    jvmToolchain {
        languageVersion.set(JavaLanguageVersion.of(17))
    }

    compose.desktop {
        compilerOptions {
            jvmTarget.set(JvmTarget.JVM_17)
        }

        application {
            mainClass = "org.baja.dashboard.ApplicationKt"

            /*
             * Configures the build to target the Windows and Debian platforms,
             * and sets the application details.
             */
            nativeDistributions {
                targetFormats(TargetFormat.Msi, TargetFormat.Deb)
                packageName = "Dashboard"
                packageVersion = "1.0.0"
                description = "Compose UI Dashboard App"
            }

            /*
             * Includes the ~/build/cpp directory into the compilation paths,
             * This directory should include the *.so files that are generated
             * when compiling the JNI library files.
             */
            jvmArgs += listOf(
                "-Djava.library.path=${buildDir}/cpp/"
            )
        }
    }
}

/**
 * Configures the C/C++ source code to be compiled as defined in the project
 * Makefile before the Kotlin code is compiled.
 */
tasks.register<Exec>("compileC++") {
    description = "Build native C/C++ CAN bridge code"
    group = "build"
    workingDir = file(projectDir)
    commandLine("make")
}

tasks.named("compileKotlin").configure {
    dependsOn("compileC++")
}
