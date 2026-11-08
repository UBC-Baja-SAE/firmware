#!/bin/sh
# If you change this please remember to chmod +x after

ip link set can0 down 2>/dev/null
ip link set can0 type can bitrate 1000000 dbitrate 2000000 fd on
ip link set can0 up

dmesg -n 1
chmod 666 /dev/fb0
chmod 666 /dev/input/event*

export LANG=C.UTF-8
export LC_ALL=C.UTF-8
export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0
export QT_QPA_FB_HIDECURSOR=1
export QT_QPA_GENERIC_PLUGINS=evdevkeyboard

chpst -u ubcbaja /home/ubcbaja/firmware/projects/central_ecu/build/central_ecu &
APP_PID=$!

cleanup() {
    kill -TERM $APP_PID 2>/dev/null
    wait $APP_PID 2>/dev/null

    EXIT_CODE=$?

    if [ $EXIT_CODE -eq 0 ] || [ $EXIT_CODE -eq 143 ]; then
        sv down central_ecu 2>/dev/null
        dmesg -n 7
        dd if=/dev/zero of=/dev/fb0 bs=1M count=4 2>/dev/null
        reset > /dev/tty1 2>/dev/null
        echo -ne '\033[?25h' > /dev/tty1 2>/dev/null
    else
        dmesg -n 7
    fi
}

trap cleanup EXIT INT TERM

wait $APP_PID