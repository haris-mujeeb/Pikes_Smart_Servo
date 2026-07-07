# PIKES Smart Servo System Workspace
This repository contains the solution developed to the evaluation task, as outlined in, as outlined in [this PDF](docs/ESP_Task_for_2nd_Round.pdf).

This repository is organized into distinct subdirectories:
1.  **[firmware/](firmware):** ESP32-S3 PlatformIO project (using ESP-IDF) implementing low-level PWM drivers, torque-based automatic calibration, and MQTT topics mapping.
2.  **[ros2_ws/](ros2_ws):** ROS 2 Control package workspace containing the MQTT driver interface, keyboard teleoperation nodes.
3.  **[docs/](docs):** Project assets and documentation sheets.

---

## Demonstration Videos
> [!IMPORTANT]
> To quickly evaluate the functionality of this solution, watch the recorded videos below demonstrating the features required for Task 1 and Task 2:
*   **Task 1 - Setup and MQTT Teleoperation Demonstration:**
    Shows the ESP32-S3 sweeping a standard servo on startup and responding to keyboard inputs from the ROS 2 teleop terminal via the MQTT broker.
    [Watch Task 1 Demo Video](docs/Task_1_Setup_and_Teleop.mp4)

*   **Task 2 - Automatic Calibration and Limit Enforcement Demonstration:**
    Shows the startup calibration sequence where the servo sweeps the door to detect limits via simulated torque spikes, followed by operational control being restricted within those boundaries.
    [Watch Task 2 Demo Video](docs/Task_2_Torque_Calibration.mp4)

---

## System Architecture

The following diagram illustrates the bi-directional flow of commands and telemetry across the entire smart servo control system:

```mermaid
graph TD
    %% Define styles for distinct zones
    classDef ros2 fill:#1E2D3D,stroke:#0A84FF,stroke-width:2px,color:#E1E8F0;
    classDef mqtt fill:#2C1E3D,stroke:#BF5AF2,stroke-width:2px,color:#F3E8FF;
    classDef esp32 fill:#1D3B2B,stroke:#30D158,stroke-width:2px,color:#E8F8EE;
    classDef hardware fill:#3D2A1C,stroke:#FF9F0A,stroke-width:2px,color:#FFF3E0;

    subgraph ROS2["ROS 2 Workspace"]
        A["teleop_twist_keyboard<br/>(Keyboard Control Node)"] -->|"/cmd_vel <br/> (geometry_msgs/Twist)"| B["twist_to_mqtt<br/>(Bridge Node)"]
        C["mqtt_to_ros<br/>(Bridge Node)"] -->|"/pikes/servo/angle<br/>/pikes/servo/torque_sim<br/>(std_msgs/Float64)"| D["rqt_plot<br/>(Telemetry Plotter)"]
    end

    subgraph Broker["HiveMQ Broker (MQTT Gateway)"]
        M_CMD["/pikes/servo/cmd_vel<br/>(JSON Command: velocity)"]
        M_TEL["/pikes/servo/telemetry<br/>(JSON Telemetry: angle, torque)"]
    end

    subgraph ESP32["ESP32-S3 Firmware (ESP-IDF)"]
        E_MQTT["MQTT Manager<br/>(Publish/Subscribe Task)"]
        E_STATE["Control State Machine<br/>(Init ➔ Calibrate ➔ Operational)"]
        E_SAFETY["Endpoint Detector<br/>(Limits Simulation & Torque Modeling)"]
        E_PWM["Servo PWM Driver<br/>(LEDC Config & Transmission scale)"]
    end

    subgraph HW["Physical Hardware"]
        H_LED["LEDC Hardware PWM Signal<br/>(0.5ms - 2.5ms Pulse @ 50Hz)"]
        H_SRV["Physical Servo Motor & Gearbox<br/>(180° or 360° Continous Servo)"]
    end

    %% Wiring ROS 2 Nodes to MQTT Topics
    B -->|"Publish JSON"| M_CMD
    M_TEL -->|"Subscribe Telemetry"| C

    %% Wiring MQTT Topics to ESP32 Firmware
    M_CMD -->|"Subscribe JSON"| E_MQTT
    E_MQTT -->|"Publish Telemetry"| M_TEL

    %% Wiring ESP32 Internal Modules
    E_MQTT <-->|"Get Velocity / Publish Telemetry"| E_STATE
    E_STATE <-->|"Simulate Torque & Validate Bounds"| E_SAFETY
    E_STATE -->|"Command Speed / Angle"| E_PWM

    %% Wiring ESP32 to Hardware
    E_PWM -->|"Write LEDC Duty"| H_LED
    H_LED -->|"Drive Actuator"| H_SRV

    %% Fixed case-sensitivity for class applications
    class A,B,C,D ros2;
    class M_CMD,M_TEL mqtt;
    class E_MQTT,E_STATE,E_SAFETY,E_PWM esp32;
    class H_LED,H_SRV hardware;
```

---

## Project Directory Structure and Links
Click on the links below to view details and readmes for each workspace component:

### ESP32-S3 Firmware (firmware/)
Contains all the embedded C source code, configuration files, and unit tests:
*   **Detailed Firmware Guide:** [firmware/README.md](firmware/README.md)
*   **Main Entrypoint Code:** [firmware/src/main.c](firmware/src/main.c)
*   **Unity Unit Testing Guide:** [firmware/test/README.md](firmware/test/README.md)
*   **PlatformIO Project Config:** [firmware/platformio.ini](firmware/platformio.ini)

### ROS 2 Control Workspace (ros2_ws/)
Contains the high-level ROS 2 nodes and communication files:
*   **ROS 2 Guide and Setup:** [ros2_ws/README.md](ros2_ws/README.md)

### Specifications and Documents (docs/)
Contains specifications and references for the project tasks:
*   **Task Requirement Sheet (PDF):** [docs/ESP_Task_for_2nd_Round.pdf](docs/ESP_Task_for_2nd_Round.pdf)

---

## Step by Step Guide
To run and evaluate the system, follow this general sequential order:

1.  **Step 1: Check Task Details:** Understand the project requirements in the task description document ([docs/ESP_Task_for_2nd_Round.pdf](docs/ESP_Task_for_2nd_Round.pdf)).
2.  **Step 2: Flash the Firmware:** Configure your Wi-Fi details, connect the ESP32-S3, and flash the program using the guide in [firmware/README.md](firmware/README.md).
3.  **Step 3: Execute Unit Tests:** Run modular unit tests to verify the driver and endpoint safety algorithms using [firmware/test/README.md](firmware/test/README.md).
4.  **Step 4: Run the ROS 2 Environment:** Launch the MQTT broker and key-press teleop nodes following the steps in [ros2_ws/README.md](ros2_ws/README.md).