import asyncio
import time
import base64
import math
import subprocess
import random
from foxglove_websocket.server import FoxgloveServer

# Import your generated class
# Make sure you ran: protoc --python_out=. telemetry.proto
import telemetry_pb2 

def get_encoded_descriptor():
    """Generates the binary descriptor Foxglove needs to decode your specific .proto"""
    subprocess.run(["protoc", "--descriptor_set_out=telemetry.bin", "telemetry.proto"])
    with open("telemetry.bin", "rb") as f:
        return base64.b64encode(f.read()).decode("ascii")

def generate_fake_ecu(t, offset):
    """Generates jittery IMU and Suspension data for a single corner"""
    ecu = telemetry_pb2.ECU()
    # Simulate suspension travel (LinPot) with some noise
    ecu.scaled_linpot = int(500 + 100 * math.sin(t + offset) + random.randint(-10, 10))
    # Simulated Strain Gauges
    ecu.scaled_strain1 = int(200 * math.cos(t * 0.5 + offset))
    # IMU Quaternions (Simplified placeholder logic)
    ecu.x, ecu.y, ecu.z, ecu.w = 0.0, 0.0, 0.0, 1.0 
    return ecu

async def main():
    schema_base64 = get_encoded_descriptor()

    async with FoxgloveServer("0.0.0.0", 8765, "UBC Baja Telemetry") as server:
        chan_id = await server.add_channel({
            "topic": "vehicle_state",
            "encoding": "protobuf",
            "schemaName": "vehicle.BajaTelemetry", # Note the package name prefix
            "schema": schema_base64,
        })

        print("🏁 Baja Telemetry Server Live: ws://localhost:8765")
        
        start_time = time.time()
        while True:
            now = time.time()
            t = now - start_time
            
            msg = telemetry_pb2.BajaTelemetry()
            
            # 1. Populate Suspension/ECU data for each corner
            msg.fr.CopyFrom(generate_fake_ecu(t, 0))
            msg.fl.CopyFrom(generate_fake_ecu(t, 0.5))
            msg.rr.CopyFrom(generate_fake_ecu(t, 1.0))
            msg.rl.CopyFrom(generate_fake_ecu(t, 1.5))

            # 2. Simulate Engine/Drive Data
            # Speed ramps up then loops
            msg.scaled_speed = int((t % 20) * 5) 
            msg.rpm = int(2000 + (t % 5) * 800)

            # 3. Static GPS (Near UBC Point Grey)
            msg.scaled_latitude = 492606 # 49.2606
            msg.scaled_longitude = -1232460 # -123.2460
            
            # 4. Send to Foxglove
            serialized_msg = msg.SerializeToString()
            await server.send_message(chan_id, time.time_ns(), serialized_msg)
            
            await asyncio.sleep(0.05) # 20Hz for smoother visualization

if __name__ == "__main__":
    asyncio.run(main())