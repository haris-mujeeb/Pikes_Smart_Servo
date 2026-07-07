# ROS 2 Control Workspace
It contains the code required to run the custom `ros2_control` hardware interface and the keyboard teleoperation nodes, connecting them to the ESP32-S3 over the MQTT broker network.

For overall project layout, check the [Main README.md](../README.md).

---

## ROS 2 System Components

The workspace coordinates three primary tasks:

1.  **MQTT Bridge Integration:** Translates high-level control outputs to JSON frames (`{"velocity": X}`) on topic `/pikes/servo/cmd_vel`, and receives status updates from `/pikes/servo/torque` (`{"angle": A, "torque": T}`).
2.  **`ros2_control` Hardware Interface:** Utilizes a custom `SystemInterface` in C++ to treat the remote ESP32 servo topics as standard joint status registers (reading and writing values dynamically).
3.  **Keyboard Teleop Node (`servo_teleop`):** Allows users to send raw rotational speed commands interactively.

---

## How to Run the ROS 2 Stack (Docker Setup)

For seamless deployment and to avoid local ROS 2 installation dependencies, we recommend running the system inside a Docker container:

### 1. Build the Docker Image
Navigate to this directory (`ros2_ws`) and build the container image containing all dependencies (such as Paho MQTT and ROS 2 desktop libraries):
```bash
docker build -t pikes_servo_ros2 .
```

### 2. Start the System with Docker Compose
Run the stack (which starts both a local Eclipse Mosquitto MQTT broker and the ROS 2 control nodes):
```bash
docker-compose up
```

### 3. Start the Keyboard Teleoperation CLI
Open a separate terminal window, attach into the running container, and start the keyboard controller:
```bash
# Attach into container
docker exec -it pikes_ros2_container bash

# Launch keypress monitor
ros2 run servo_teleop keyboard_control
```

*   Use the **Left / Right arrow keys** to change rotation direction.
*   Use the **+ / - keys** to increase or decrease speed.

---
