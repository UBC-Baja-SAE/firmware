@echo off
set "SCRIPT_DIR=%~dp0"

openocd ^
  -f "%SCRIPT_DIR%openocd.cfg" ^
  -c "gdb_port disabled" ^
  -c "tcl_port disabled" ^
  -c "telnet_port disabled" ^
  -c "init" ^
  -c "reset run" ^
  -c "sleep 500" ^
  -c "rtt setup 0x20000000 0x20000 \"SEGGER RTT\"" ^
  -c "rtt start" ^
  -c "rtt polling_interval 1" ^
  -c "rtt channels" ^
  -c "rtt server start 19021 1"

pause