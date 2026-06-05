import subprocess
import time
from gpiozero import Button

COMPOSE_DIR = "/home/ubcbaja/firmware/projects/ros2"
CONTAINER_NAME = "ros2-ros2-1"

print("Starting Baja Button Watcher...")

while True:
    # 1. Claim the pin
    start_btn = Button(22, pull_up=True, bounce_time=0.1)
    print("Watching for start button on GPIO 22...")

    # 2. Pause the script here until the button is physically pressed
    start_btn.wait_for_press()

    # 3. THE MAGIC TRICK: Close the button to release the hardware lock!
    start_btn.close()
    print("Button pressed! Pin 22 lock released.")

    # 4. Start the container
    print("Starting ROS 2 Container...")
    subprocess.run(["docker", "compose", "up", "-d"], cwd=COMPOSE_DIR)

    # Give Docker a few seconds to spin up and claim the pin
    time.sleep(3)

    # 5. Suspend this host script until the container completely shuts down
    print("Container is running. Host script going to sleep...")
    subprocess.run(["docker", "wait", CONTAINER_NAME])

    # 6. The container has died. Loop back and claim the pin again!
    print("Container shut down detected. Reclaiming Pin 22...")
    time.sleep(1) # Give the OS a second to fully clear the hardware lock