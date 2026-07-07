# PlatformIO Unit Testing Suite

This directory contains modular, automated test suites built using the **Unity** testing framework configured within PlatformIO. These suites validate low-level driver initialization, the torque-based endpoint detection algorithm, logging output, and bi-directional MQTT communications.

For overall project architecture, setup, and deployment guidelines, refer to the [Main README.md](../../README.md).

---

## How to Run the Tests

Any beginner can easily run these tests with a single command. Follow these steps:

### 1. Hardware Prerequisite
Ensure your ESP32-S3 Development Board is connected to your host PC via its USB port.

### 2. Execution Commands

*   **Run all test suites sequentially:**
    Make sure you are in the `firmware` directory:
    ```bash
    cd firmware
    pio test
    ```

*   **Run a specific test suite:**
    To run only a single test suite, specify its folder name using the `-f` filter option:
    ```bash
    # Run only the servo sweeps
    pio test -f test_servo

    # Run only the endpoint calibration math tests
    pio test -f test_endpoint
    ```

*   **Run tests with Verbose Logging (Recommended for Interviewers):**
    To see full stdout logs and ESP-IDF debug macros (`ESP_LOGI`, `ESP_LOGW`, etc.) in real time, append the `-v` flag:
    ```bash
    pio test -f test_servo -v
    ```

---

## Test Suites in Detail

The project contains four dedicated test folders. Below is a detailed breakdown of each suite, complete with clickable source code references:

### 1. Servo Driver Verification (test_servo)
*   **Source File:** [test_servo/test_main.c](test_servo/test_main.c)
*   **Objective:** Verifies that the ESP32-S3 LEDC (Light Emitting Diode Controller) PWM driver initializes correctly and can actuate a physical servo motor.
*   **Key Test Cases:**
    *   [test_servo_initialization](test_servo/test_main.c#L11): Asserts that [servo_init](../lib/servo_driver/src/servo_driver.c#L40) completes with `ESP_OK`.
    *   [test_servo_sweep](test_servo/test_main.c#L15): Drives the physical servo through a set sequence of angles ($10^\circ \to 90^\circ \to 180^\circ \to 90^\circ \to 10^\circ$) to verify steady operation and correct pulse-width math.

### 2. Endpoint Detection and Calibration Routine (test_endpoint)
*   **Source File:** [test_endpoint/test_main.c](test_endpoint/test_main.c)
*   **Objective:** Tests the core logic of the automatic calibration state machine (solving Task 2 of the PDF).
*   **Key Test Cases:**
    *   [test_torque_simulation](test_endpoint/test_main.c#L14): Triggers [endpoint_calibrate](../src/endpoint_detector.c#L34) and asserts that the randomized physical limit boundaries simulate a range of exactly 60 degrees.
    *   [test_movement_restriction](test_endpoint/test_main.c#L24): Verifies that [endpoint_is_movement_allowed](../src/endpoint_detector.c#L73) returns `true` for positions inside the safe calibrated area and `false` for any positions outside the min/max limits.

### 3. Native USB Logging Verification (test_logs)
*   **Source File:** [test_logs/test_main.c](test_logs/test_main.c)
*   **Objective:** Verifies that print/log streams are routing correctly through the ESP32-S3's internal USB Serial/JTAG hardware.
*   **Key Test Cases:**
    *   [test_logging_output](test_logs/test_main.c#L12): Fires different levels of ESP-IDF logging macros (`ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE`) to demonstrate terminal capability.

### 4. Interactive MQTT Network Loopback Verification (test_mqtt)
*   **Source File:** [test_mqtt/test_main.c](test_mqtt/test_main.c)
*   **Objective:** Asserts that the WiFi stack connects successfully and bi-directional communications over the MQTT broker are operational.
*   **Key Test Cases:**
    *   [test_mqtt_teleop_handshake](test_mqtt/test_main.c#L17): Starts the local WiFi connection, initializes the MQTT broker hook, and polls for inbound commands.
*   **Critical Running Requirement:**
    The ESP32 firmware requires interactive network loopback validation. **You must run the accompanying Python helper script on your host computer concurrently** with the ESP32 test execution:
    1.  Open a terminal on your host PC.
    2.  Start the Python client script:
        ```bash
        python3 firmware/test/test_mqtt/test_mqtt.py
        ```
    3.  This script connects to the public broker `broker.hivemq.com` and continuously publishes velocity payloads (`{"velocity": 15.0}`) to `/pikes/servo/cmd_vel`.
    4.  Run `pio test -f test_mqtt` on the ESP32-S3. The test will capture these payloads and verify that the firmware correctly decodes commands, preventing the test from timing out and failing.
