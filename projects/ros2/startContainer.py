import subprocess
import signal
from gpiozero import Button

# The directory where your docker-compose.yml lives
COMPOSE_DIR = "/home/ubcbaja/firmware/projects/ros2"
# The name of your container (usually foldername-servicename-1)
CONTAINER_NAME = "ros2-ros2-1"

def is_container_running():
    # Asks Docker if the specific container is currently active
    cmd = ["docker", "ps", "-q", "-f", f"name={CONTAINER_NAME}"]
    result = subprocess.run(cmd, capture_output=True, text=True)
    # If stdout has text, the container is running
    return len(result.stdout.strip()) > 0

def on_button_press():
    if not is_container_running():
        print("Container is OFF. Starting ROS 2...")
        # Start the container from the correct directory
        subprocess.run(["docker", "compose", "up", "-d"], cwd=COMPOSE_DIR)
    else:
        # Container is already ON.
        # Do nothing. Let the GUI handle the menu clicks and long-press shutdown!
        pass

# Bind to the same pin as the GUI (Pin 22)
start_btn = Button(22, pull_up=True, bounce_time=0.1)
start_btn.when_pressed = on_button_press

print("Watching for start button on GPIO 22...")
signal.pause()