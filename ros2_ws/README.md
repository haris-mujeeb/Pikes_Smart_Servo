# ROS 2 Workspace (servo_teleop)

This workspace contains the implementation of the `servo_teleop` ROS 2 package. It establishes a bi-directional bridge between ROS 2 topics and a remote ESP32-S3 smart servo controller using an MQTT broker, and provides keyboard teleoperation and telemetry plotting tools.

For the overall system layout and ESP32 firmware details, check the [Main README.md](../README.md).


---

## ROS 2 System Components

The workspace coordinates the following components via its main launch file `servo_teleop.launch.py`:

1.  **MQTT Command Bridge (`twist_to_mqtt`):** Subscribes to the `/cmd_vel` ROS 2 topic, scales the keyboard angular velocity commands, and publishes JSON payloads (`{"velocity": X}`) to the MQTT topic `/pikes/servo/cmd_vel`.
2.  **MQTT Telemetry Bridge (`mqtt_to_ros`):** Connects to the MQTT broker, subscribes to the telemetry topic `/pikes/servo/telemetry`, parses JSON status updates (angle and torque), and republishes them as standard ROS 2 `std_msgs/Float64` messages on `/pikes/servo/angle` and `/pikes/servo/torque_sim`.
3.  **Keyboard Teleoperation CLI (`teleop_twist_keyboard`):** Runs the standard teleoperation node inside an external `xterm` popup window to capture keyboard controls without blocking the main terminal stdout.
4.  **Telemetry Visualizer (`rqt_plot`):** Opens a graphical interface displaying live plots of the servo angle and torque telemetry.

---

## How to Run the ROS 2 Stack (Docker Setup)

Because the ROS 2 stack uses GUI tools (`xterm` and `rqt_plot`) inside the Docker container, running it requires **X11 forwarding** so the container can draw windows on your host screen.

Follow these steps to Dockerize your workspace, build the image, and configure GUI access:

### 1. Build the Docker Image
Navigate to the `ros2_ws` directory and build the container image. This packages your source code, installs `paho-mqtt` and `xterm`, and builds the ROS 2 workspace inside the image:
```bash
docker build -t pikes_smart_servo .
```

### 2. Allow Local GUI Connections
Before starting the container, authorize the host's X server to accept GUI window connections from local clients (like the container). Run this command on your host terminal:
```bash
xhost +local:root
```

### 3. Run the Docker Container
Launch the container with network sharing (for MQTT broker communications) and the display/socket bindings required for X11 forwarding:
```bash
docker run -it --rm \
    --net=host \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    pikes_smart_servo
```

Once executed:
- The main terminal will log connection status updates from the MQTT bridges.
- An `xterm` window will pop up to capture keyboard input (use standard `teleop_twist_keyboard` keys to command movement).
- An `rqt_plot` window will display the live telemetry graphs.

---

## How to Run the ROS 2 Stack Locally (Normal Setup)

If you have a local ROS 2 installation (e.g., ROS 2 Humble on Ubuntu 22.04), you can build and run the workspace directly on your host machine without Docker:

### 1. Install Dependencies
Ensure you have the required ROS 2 packages and Python libraries installed on your host system:
```bash
sudo apt update
sudo apt install -y python3-pip xterm ros-humble-desktop ros-humble-teleop-twist-keyboard ros-humble-rqt-plot
pip3 install paho-mqtt
```

### 2. Build the Workspace
Navigate to the root of your workspace (`ros2_ws`) and compile the packages:
```bash
colcon build
```

### 3. Source the Environment
Source both the global ROS 2 setup and your newly built local workspace overlay:
```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
```

### 4. Launch the System
Run the launch file to start all nodes, including the MQTT bridges, the keyboard input console, and the telemetry plot:
```bash
ros2 launch servo_teleop servo_teleop.launch.py
```

---
