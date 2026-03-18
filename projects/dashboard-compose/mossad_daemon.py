import paho.mqtt.client as mqtt
import subprocess
import random
import time
import threading

MQTT_BROKER = "localhost" # Or your Pi's IP
TOPIC = "mossad"

string = "you"

import os


def run_scare(name="YOU"):
    subprocess.run(["python3", "/home/ubcbaja/firmware/projects/dashboard-compose/mossad.py", name])

def on_message(client, userdata, msg):
    # Trigger whenever any message hits the mossad topic
    target = msg.payload.decode()
    string = target
    run_scare(target)

# # Random timer thread
# def random_trigger():
#     while True:
#         # Wait between 5 to 15 minutes for the random scare
#         time.sleep(random.randint(900, 1800))
#         run_scare(string)

# Setup MQTT
client = mqtt.Client()
client.on_message = on_message
client.connect(MQTT_BROKER, 1883, 60)
client.subscribe(TOPIC)

# Start random thread
# threading.Thread(target=random_trigger, daemon=True).start()

client.loop_forever()