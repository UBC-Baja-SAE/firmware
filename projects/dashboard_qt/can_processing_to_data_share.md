# CAN Processing to Data Sharing - Qt Dashboard

The dashboard's data processing pipeline uses a multi-threaded C++ architecture with Qt for UI and optional Foxglove SDK for live telemetry streaming.

## Architecture Components

### CAN Bridge (`can_bridge.h/cpp`)
- Reads CAN frames from SocketCAN interface (Linux) or UART
- Stores raw frame data in `observed_data[2048]` array indexed by CAN ID
- Provides `getData(int id)` method for type-aware data extraction:
  - 6-byte IMU data (accel/gyro)
  - 2-byte sensors (suspension/strain)
  - 4-byte sensors (tach/speedo)
  - 8-byte sensors (all others)
- Runs dedicated thread for real-time CAN message reception

### CAN Processor (`can_processor.h/cpp`)
- Polls `observed_data` array at 100Hz for changes
- Parses CAN messages by ID and updates DataManager
- Supports all vehicle sensors:
  - Dashboard: tachometer, speedometer, temperature, fuel
  - ECU (per corner): suspension travel, strain gauges, IMU
  - GPS: position, speed, fix status
- Uses change detection to minimize redundant processing

### Data Manager (`data_manager.h/cpp`)
- Thread-safe singleton using mutex protection
- Stores vehicle state in protobuf format (`test.Data`)
- Provides typed setter methods for all sensor data
- Consumers call `getLatestData()` to retrieve current state

### MCAP Logger (`mcap_logger.h/cpp`)
- **Dual-mode operation**: File writing + WebSocket streaming
- Polls DataManager at configurable rate (default 100Hz)
- **With Foxglove SDK** (recommended):
  - Real-time WebSocket streaming to Foxglove Studio
  - Simultaneous MCAP file recording
  - Proper MCAP format with schema registration
- **Without Foxglove SDK** (fallback):
  - Basic binary file writing (length-prefixed protobuf)

### Qt Display (`mainwindow.h/cpp`)
- QTimer-based UI updates at 30Hz
- Fetches latest data from DataManager
- Updates widgets with sensor values
- Manages lifecycle of all background threads

## Data Flow Diagram

```
┌──────────────┐
│  CAN Socket  │ (can0 interface)
└──────┬───────┘
       │
       v
┌──────────────┐
│  CAN Bridge  │ Thread 1: SocketCAN reader
│observed_data │ Array of raw CAN frames [2048]
└──────┬───────┘
       │
       v
┌───────────────┐
│ CAN Processor │ Thread 2: 100Hz polling
│ processFrame()│ Parse CAN ID → Update DataManager
└──────┬────────┘
       │
       v
┌──────────────────┐
│  Data Manager    │ Singleton with mutex
│   (Protobuf)     │ Thread-safe getLatestData()
└────┬───────┬─────┘
     │       │
     │       └──────────────────────────┐
     │                                  │
     v                                  v
┌────────────┐                  ┌──────────────────┐
│ Qt Display │ Thread 3: Main   │   MCAP Logger    │ Thread 4: 100Hz
│  (30Hz)    │ QTimer updates   │ (Foxglove SDK)   │ Data recording
└────────────┘                  └──────┬───────────┘
                                       │
                     ┌─────────────────┼─────────────────┐
                     │                 │                 │
                     v                 v                 v
              ┌─────────────┐   ┌────────────┐   ┌────────────┐
              │ MCAP File   │   │ WebSocket  │   │  Foxglove  │
              │ Recording   │   │  Server    │   │   Studio   │
              │ (Disk)      │   │ :8765      │   │  (Remote)  │
              └─────────────┘   └────────────┘   └────────────┘
                                  ws://0.0.0.0:8765
```

## Building with Foxglove SDK

### Prerequisites

1. **Install Foxglove SDK** (C++ wrapper around Rust core):
   ```bash
   # Download from GitHub releases
   wget https://github.com/foxglove/foxglove-sdk/releases/latest/download/foxglove-sdk-linux-x64.tar.gz
   tar -xzf foxglove-sdk-linux-x64.tar.gz
   sudo cp -r foxglove-sdk/include/* /usr/local/include/
   sudo cp -r foxglove-sdk/lib/* /usr/local/lib/
   ```

2. **Enable in CMake**:
   ```cmake
   # In CMakeLists.txt, uncomment:
   option(USE_FOXGLOVE_SDK "Enable Foxglove WebSocket streaming" ON)
   if(USE_FOXGLOVE_SDK)
       find_package(foxglove REQUIRED)
       add_compile_definitions(USE_FOXGLOVE_SDK)
       target_link_libraries(dashboard_qt PRIVATE foxglove::foxglove)
   endif()
   ```

3. **Build**:
   ```bash
   mkdir build && cd build
   cmake -DUSE_FOXGLOVE_SDK=ON ..
   make
   ```

### Without Foxglove SDK

The system falls back to basic binary file writing if compiled without `-DUSE_FOXGLOVE_SDK`. WebSocket streaming will be unavailable, but file recording still works.

## Usage

### Starting the Dashboard

```cpp
// In mainwindow.cpp
startCanBridge();           // Start CAN listener
startCanProcessor();        // Start CAN parser

// Three modes:
// 1. Stream only (no file)
startMcapLogger("", 100, true);

// 2. File + stream (recommended)
startMcapLogger("/tmp/dashboard_data.mcap", 100, true, "0.0.0.0", 8765);

// 3. File only
startMcapLogger("/tmp/dashboard_data.mcap", 100, false);
```

### Connecting Foxglove Studio

1. Open Foxglove Studio
2. Click "Open connection"
3. Select "Foxglove WebSocket"
4. Enter: `ws://<vehicle-ip>:8765`
5. Click "Open"

Data will stream in real-time with proper schema definitions.

## Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| CAN Bridge | can0 | SocketCAN interface |
| CAN Processor Rate | 100Hz | Polling frequency |
| MCAP Logger Rate | 100Hz | Recording/streaming rate |
| Qt Display Rate | 30Hz | UI refresh rate |
| WebSocket Host | 0.0.0.0 | Listen on all interfaces |
| WebSocket Port | 8765 | Foxglove default port |

## Thread Safety

- **CAN Bridge**: Lock-free writes to `volatile` array
- **CAN Processor**: Read-only access to `observed_data`
- **Data Manager**: Mutex-protected protobuf updates
- **MCAP Logger**: Read-only via `getLatestData()` (creates copy)
- **Qt Display**: Read-only via `getLatestData()` (creates copy)

No data races between threads; DataManager mutex is the only synchronization point.