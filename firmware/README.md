# PIKES Smart Servo Control System
This repository contains the ESP32-S3 firmware and ROS 2 integration for an intelligent smart servo system. 
The system bridges a high-level ROS 2 environment with a low-level physical ESP32-S3 microcontroller using an MQTT communication gateway, controling an actual standard 180-degree servo motor.

---

## Quick Navigation Links

*   **Firmware Entrypoint:** [src/main.c](src/main.c)
*   **PWM Driver Library:** [lib/servo_driver/src/servo_driver.c](lib/servo_driver/src/servo_driver.c) & [Header](lib/servo_driver/include/servo_driver.h)
*   **Calibration & Torque Logic:** [src/endpoint_detector.c](src/endpoint_detector.c) & [Header](include/endpoint_detector.h)
*   **MQTT & WiFi Manager:** [src/mqtt_manager.c](src/mqtt_manager.c) & [Header](include/mqtt_manager.h)
*   **Unit Testing Guide:** [test/README.md](test/README.md)

---

## Demonstration Videos

> [!IMPORTANT]
> The demonstration videos showcase the completed implementation of Task 1 (System Setup & Control) and Task 2 (Automatic Endpoint Calibration) on the physical ESP32-S3 and servo motor setup.

### 1. Task 1: System Setup, PWM Sweep & MQTT Teleoperation
This video demonstrates the initial ESP32-S3 system setup, including the standard 180 degree sweep startup test and real-time keyboard control of the servo rotation over the MQTT bridge using a custom ROS 2 interface.
*   **Watch Video:** [Task_1_Setup_and_Teleop.mp4](../docs/Task_1_Setup_and_Teleop.mp4)

### 2. Task 2: Automatic Torque-Based Calibration & Movement Restriction
This video highlights the calibration state machine running on startup. The ESP32-S3 sweeps the door mechanism until a torque spike is detected, saves the randomized physical limits (simulated within 60 degrees), and blocks any subsequent keyboard teleoperation that attempts to exceed these safe operational limits.
*   **Watch Video:** [Task_2_Torque_Calibration.mp4](../docs/Task_2_Torque_Calibration.mp4)

---

## How to Run the Project (Quick Start Guide)

Follow these steps to deploy and run the entire system from scratch.

### 1. Configure WiFi and MQTT Broker Address
Open the main firmware configuration in [src/main.c](src/main.c#L17-L19) and configure your network details:
```c
#define WIFI_SSID       "Your_WiFi_SSID"
#define WIFI_PASS       "Your_WiFi_Password"
#define MQTT_BROKER_URI "mqtt://your_mqtt_broker_ip"
```
*Note: You can use a public broker like HiveMQ (`mqtt://broker.hivemq.com`) for testing.*

### 2. Build and Flash the Firmware (PlatformIO)
1. Install VS Code and the **PlatformIO** extension.
2. Open the project root folder in VS Code.
3. Connect your ESP32-S3 Development Board via USB.
4. Open the PlatformIO terminal or use the bottom toolbar to run:
   ```bash
   # Build the firmware
   pio run
   # Upload to ESP32-S3 and open the serial monitor
   pio run --target upload --target monitor
   ```

### 3. Run the ROS 2 Environment (Docker or Local)
To launch the MQTT bridges, keyboard controller, and telemetry plotting interface, refer to the detailed instructions in [ros2_ws/README.md](../ros2_ws/README.md):
*   **Docker Setup:** Run using `docker build -t pikes_smart_servo .` and run with X11 forwarding enabled to allow the `xterm` and `rqt_plot` windows to display on your host screen.
*   **Local Setup:** Run locally by compiling with `colcon build`, sourcing, and running `ros2 launch servo_teleop servo_teleop.launch.py`.

### 5. Running Unit Tests
You can validate components (PWM, Endpoints, Logging, MQTT) using the built-in Unity test framework:
```bash
# Run all tests
pio test
# Run with verbose logs to inspect custom logging outputs
pio test -v
```
*For detailed testing instructions, prerequisites, and Python script configurations, refer to the [test/README.md](test/README.md) file.*

---

## Deep Dive: System Architecture & Implementation Details

To solve the requirements defined in the [ESP Task Description](../docs/ESP_Task_for_2nd_Round.pdf), the firmware employs a clean, modular structure. For the complete system architecture diagram illustrating the interaction between ROS 2, MQTT, and the ESP32 firmware, see the [Main System Architecture](../README.md#system-architecture).

---

### Firmware Modules Explained

#### 1. Low-Level PWM Control ([lib/servo_driver/src/servo_driver.c](lib/servo_driver/src/servo_driver.c))
The servo driver maps target degrees or rotational velocities to a high-resolution hardware PWM signal:
*   [servo_init](lib/servo_driver/src/servo_driver.c#L40): Configures the ESP32-S3 LEDC peripheral with a `50Hz` frequency and a `13-bit` duty resolution (max duty value of `8191`).
*   [servo_set_angle](lib/servo_driver/src/servo_driver.c#L68): For positional 180-degree servos, converts $0^\circ \dots 180^\circ$ inputs to pulse widths between `0.5ms` and `2.5ms` to set the duty cycle.
*   [servo_set_speed](lib/servo_driver/src/servo_driver.c#L83): For continuous 360-degree servos, maps $-100\% \dots +100\%$ speed values to pulse widths between `1.0ms` (full reverse) and `2.0ms` (full forward), with `1.5ms` acting as the stop signal.

#### 2. Transmission Gear Ratio Handling
As defined in Task 1, a mechanical gearbox with a **40:15 ratio** is placed between the motor shaft and the door hinge.
*   The firmware maps target door movements to motor movements using the [servo_apply_transmission_ratio](lib/servo_driver/src/servo_driver.c#L98) function:
    $$\text{Motor Target} = \text{Door Target} \times \left(\frac{40}{15}\right)$$

#### 3. Automatic Calibration & Torque Simulation ([src/endpoint_detector.c](src/endpoint_detector.c))
To accommodate variations in door sizes across different robots, the system uses a torque-based automatic calibration state machine:
*   **Limits Setup:** [endpoint_mock_init](src/endpoint_detector.c#L15) generates random physical limits (door open and closed) representing a door frame opening range of exactly 60 degrees ($true\_max - true\_min = 60^\circ$).
*   **Torque Modeling:** [endpoint_simulate_torque](src/endpoint_detector.c#L27) returns `TORQUE_SPIKE` if the current angle is outside the physical boundaries, mimicking the motor stalling as it hits the door frame. Otherwise, it returns `TORQUE_NORMAL`.
*   **Calibration Routine:** On startup, the firmware transitions through three calibration states in [app_main](src/main.c#L73):
    1.  `STATE_CALIBRATE_MIN`: Rotates the servo counter-clockwise (reducing the angle) until a torque spike exceeds the collision threshold (`TORQUE_THRESHOLD`), registering the `detected_min` limit.
    2.  `STATE_CALIBRATE_MAX`: Returns to center, then rotates clockwise (increasing the angle) until another torque spike occurs, registering the `detected_max` limit.
    3.  `STATE_OPERATIONAL`: The calibration boundaries are locked in, and the safe range is logged.
*   **Movement Restriction:** Once calibration is complete, the operational loop verifies all inputs against the calibrated limits using [endpoint_is_movement_allowed](src/endpoint_detector.c#L73). If the teleop command would drive the servo out of bounds, the velocity is clamped to `0.0f` to prevent mechanical damage.

#### 4. MQTT Gateway Protocol ([src/mqtt_manager.c](src/mqtt_manager.c))
The gateway runs asynchronously, mapping hardware parameters to MQTT broker topics:
*   **Inbound Control:** Subscribes to `/pikes/servo/cmd_vel` to receive incoming velocity inputs in JSON format: `{"velocity": X}`. These values are parsed using `cJSON` and updated in a thread-safe variable.
*   **Outbound Telemetry:** Periodically publishes telemetry payload to `/pikes/servo/torque` containing JSON details: `{"angle": A, "torque": T}`. This allows live plotting in ROS tools such as Foxglove Studio or `rqt_plot`.

---

## Hardware Requirements
*   **Microcontroller:** ESP32-S3 (Development Board)
*   **Actuator:** Physical standard 180° servo motor (e.g., HiTEC HS-311 or equivalent)
*   **Network:** 2.4GHz WiFi Connection
*   **MQTT Broker:** Public HiveMQ or local Mosquitto server

---

## VS Code & IntelliSense Setup

If you are using the **clangd** extension for C/C++ IntelliSense instead of the default Microsoft extension, you need to generate a compilation database so it can locate the ESP-IDF framework headers.

1.  Disable the default IntelliSense engine in `.vscode/settings.json`:
    ```json
    "C_Cpp.intelliSenseEngine": "disabled"
    ```
2.  Open the **PlatformIO Core CLI** (using the terminal icon in the bottom toolbar) and generate the database:
    ```bash
    pio run -t compiledb
    ```
3.  Point `clangd` to the newly generated database in `.vscode/settings.json`:
    ```json
    "clangd.arguments": [
        "--compile-commands-dir=.pio/build/esp32-s3-devkitc-1"
    ]
    ```
4.  Restart the `clangd` language server via the VS Code Command Palette.
