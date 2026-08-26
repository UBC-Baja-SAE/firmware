Run the following command with openOCD on the modded St-Link V2:
```bash
/opt/homebrew/Cellar/open-ocd/0.12.0_1/bin/openocd \
  -s /opt/homebrew/Cellar/open-ocd/0.12.0_1/share/openocd/scripts \
  -f /Users/bfrzn/git/firmware/projects/rear_dbc/openocd.cfg \
  -c "gdb_port disabled" \
  -c "tcl_port disabled" \
  -c "telnet_port disabled" \
  -c "init" \
  -c "reset run" \
  -c "sleep 500" \
  -c "rtt setup 0x20000000 0x20000 \"SEGGER RTT\"" \
  -c "rtt start" \
  -c "rtt server start 19021 1"
```

Make sure to set the ip to 127.0.0.1 and port to 19021 in SystemView