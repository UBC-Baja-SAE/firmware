while True:
    # 1. Claim the pin
    start_btn = Button(22, pull_up=True, bounce_time=0.1)
    print("Watching for start button on GPIO 22...")

    # 2. Pause the script here until the button is physically pressed
    start_btn.wait_for_press()

    # 3. Release the lock and FORCE Python to forget the button
    start_btn.close()
    del start_btn
    print("Button pressed! Pin 22 lock released.")

    # ---> THE FIX: Give the Linux kernel 1.5 seconds to fully free the GPIO line
    time.sleep(1.5)

    # 4. Start the container
    print("Starting ROS 2 Container...")
    subprocess.run(["docker", "compose", "up", "-d"], cwd=COMPOSE_DIR)

    # Give Docker a few seconds to spin up
    time.sleep(3)

    # 5. Suspend this host script until the container completely shuts down
    print("Container is running. Host script going to sleep...")
    subprocess.run(["docker", "wait", CONTAINER_NAME])

    # 6. The container has died. Loop back and claim the pin again!
    print("Container shut down detected. Reclaiming Pin 22...")

    # Give the OS a second to clear the container's hardware lock
    time.sleep(2)