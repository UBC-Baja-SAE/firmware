# Compose Dashboard

## Overview

This project is an **automotive dashboard application** built using 
[**Kotlin**](https://kotlinlang.org/) 
and
[**Compose Multiplatform**](https://www.jetbrains.com/compose-multiplatform/),
and designed for the Raspberry Pi 5. The dashboard aims to provide 
visualisation for data received from sensors on the vehicle using CAN bus 
communication.

## Requirements

Kotlin targets the
[JVM](https://en.wikipedia.org/wiki/Java_virtual_machine),
and compiles to Java bytecode, hence this application has similar dependencies
to most Java-based projects. Note that you need to set the `JAVA_HOME` 
environment variable to the file path of the **correct JDK/JRE** for the Pi or 
your computer if it is already set to a different Java version. 

### Raspberry Pi

UBC Baja utilises a 
[Raspberry Pi 5](https://www.raspberrypi.com/products/raspberry-pi-5/) 
running the official
[Raspberry Pi OS](https://www.raspberrypi.com/software/operating-systems/#raspberry-pi-os-64-bit),
which is a derivative of
[Debian 12 (Bookworm)](https://wiki.debian.org/DebianBookworm). 
To run the application on the physical dashboard, ensure the following is 
set-up on the Pi:
- Install
  [JRE 17](https://packages.debian.org/bookworm/openjdk-17-jre),
  the default (and latest _stable_) version of Java for Bookworm (as of January 
  2025). It is worth noting that only the Java _Runtime_ is mandatory to _run_ 
  the application. Ensure that the `JAVA_HOME` variable is set correctly.
- The Raspberry Pi 5 may have compatibility issues with the rendering of the 
  Compose libraries. Set the environment variable on the Pi as shown 
  [here](https://github.com/JetBrains/skiko/issues/838#issuecomment-1997275861), 
  and run the following command in the Pi shell.
  - `export SKIKO_RENDER_API=SOFTWARE`
  
### Your Computer

- Install 
  [JDK 17](https://www.oracle.com/ca-en/java/technologies/downloads/)
  for your operating system.
- (optional) Download
  [IntelliJ IDEA](https://it.ubc.ca/services/desktop-print-services/software-licensing/free-open-source-software),
  an IDE with built-in support for Java and Kotlin development. This software is
  developed by JetBrains (the company that created Kotlin), and as UBC students,
  we get free access to IntelliJ IDEA Ultimate.

## Access

The Raspberry Pi can be interfaced with directly using the USB ports (or 
Bluetooth) to connect a mouse and keyboard.

Additionally, the Pi can be accessed using the
[SSH](https://en.wikipedia.org/wiki/Secure_Shell)
protocol by simply connecting the Pi to your computer and accessing it remotely 
through the terminal. Developers should be comfortable with utilising the 
command-line interface.

### Configurations

The passwords and addresses to access the Pi via SSH is configured as:

key|value
-|-
`username`|ubcbaja
`hostname`|bajapi.local
`address` |192.168.1.2
`password`|juanisabozo

Enter the password when prompted.

### Connecting over WiFi

When the Raspberry Pi 5 and your computer are connected to the **same** wireless
network, the Pi can be accessed through LAN by entering the following in your
terminal:
```
ssh [username]@[hostname]
```
> This **does not work** through mobile hotspots or any of UBC's networks.

### Connecting over Ethernet

When connected to the Raspberry Pi with an ethernet cable, you can connect to
the Pi's static IP address by entering the following in your terminal:
```
ssh [username]@[address]
```
Because this only requires a physical connection, this can also be used on the
field.

### GitHub Authentication

When utilising GitHub through the Raspberry Pi over SSH, because it is a
**shared** device, please remember to **delete SSH keys**, and re-configure your
GitHub settings when/if you leave the team. You can create SSH keys by following
[this guide](https://gist.github.com/xirixiz/b6b0c6f4917ce17a90e00f9b60566278).

### Programming Remotely

Because the Raspberry Pi can be accessed using SSH, firmware can be modified,
launched, or created by utilising terminal-based text editors. Currently, the
installed text editors are
[nano](https://www.nano-editor.org/)
and
[vim](https://www.vim.org/).

As the text editors are installed on the Pi itself, do not modify the `.bashrc`
or `.vimrc` configuration files without documenting the changes, as they will
adjust the settings for everyone programming directly through SSH.

> The line `export DISPLAY=:0` in the `.bashrc` file configures applications to
> display on the screen connected to the Pi, and _not_ your computer screen.

## Architecture

> All Kotlin (`*.kt`) files are relative to the directory `src/main/kotlin`
> unless specified.

The dashboard application uses Compose Multiplatform, which is a framework for 
Kotlin user-interface (UI) development which enables targeting different 
platforms, including Unix or Windows systems. Compose follows a [declarative UI programming](https://docs.flutter.dev/get-started/flutter-for/declarative) 
style, hence the contents of the screen are produced as a function of the UI's 
state, rather than being constructed component by component.
