# Compose Dashboard
###### Author: Donghwa Kim

## Overview
This project is an **automotive dashboard application** built using 
[**Kotlin**](https://kotlinlang.org/) and [**Compose Multiplatform**](https://www.jetbrains.com/compose-multiplatform/),
and designed for the Raspberry Pi 5. The dashboard aims to provide 
visualisation for data received from sensors on the vehicle using CAN bus 
communication.

## Requirements
Kotlin targets the [JVM](https://en.wikipedia.org/wiki/Java_virtual_machine),
and compiles to Java bytecode, hence this application has similar dependencies
to most Java-based projects. Note that you need to set the `JAVA_HOME` 
environment variable to the file path of the **correct JDK/JRE** for the Pi or 
your computer if it is already set to a different Java version. 
### Operation
UBC Baja utilises a [Raspberry Pi 5](https://www.raspberrypi.com/products/raspberry-pi-5/) 
running the official [Raspberry Pi OS](https://www.raspberrypi.com/software/operating-systems/#raspberry-pi-os-64-bit),
which is a derivative of [Debian 12 (Bookworm)](https://wiki.debian.org/DebianBookworm). 
To run the application on the physical dashboard, ensure the following is 
set-up on the Pi:
- Install [JRE 17](https://packages.debian.org/bookworm/openjdk-17-jre), the 
  default (and latest _stable_) version of Java for Bookworm (as of January 
  2025). It is worth noting that only the Java _Runtime_ is mandatory to _run_ 
  the application. Ensure that the `JAVA_HOME` variable is set correctly.
- The Raspberry Pi 5 may have compatibility issues with the rendering of the 
  Compose libraries. Set the environment variable on the Pi as shown 
  [here](https://github.com/JetBrains/skiko/issues/838#issuecomment-1997275861), 
  and run the following command in the Pi shell.
  - `export SKIKO_RENDER_API=SOFTWARE`
### Development
- Install [JDK 17](https://www.oracle.com/ca-en/java/technologies/downloads/)
  for your operating system.
- (optional) Download [IntelliJ IDEA](https://it.ubc.ca/services/desktop-print-services/software-licensing/free-open-source-software),
  an IDE with built-in support for Java and Kotlin development. This 
  software is developed by JetBrains (the company that created Kotlin), and 
  as UBC students, we get free access to IntelliJ IDEA Ultimate.

## Setup
To develop the application

## Deployment