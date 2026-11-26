# ESP32-S3 IoT Pool Level Monitoring System

## Overview

An IoT-based pool water level monitoring system developed using an ESP32-S3-CAM and a waterproof ultrasonic sensor. The system periodically measures the water level, captures an image of the pool, and sends the results through Telegram over Wi-Fi.

This project was developed for a neighbor experiencing pool water loss who needed a low-cost alternative to frequent manual water level measurements while investigating a potentially expensive repair.

## Key Features

- Non-contact ultrasonic water level measurement
- Median filtering to reduce measurement outliers
- Calibrated water level readings in inches
- Automated Telegram notifications with image capture
- Timer-based deep sleep with a 15-minute interval
- Custom soldered perfboard with removable components
- Custom 3D-printed enclosure designed in Autodesk Fusion

## Hardware

| Component | Description |
|---|---|
| Freenove ESP32-S3-CAM | Microcontroller, camera, and Wi-Fi communication |
| JSN-SR04T | Waterproof ultrasonic sensor |
| Resistors | Voltage-level interfacing |
| Perfboard | Soldered circuit assembly |
| Female pin headers | Removable component connections |
| 3D-printed enclosure | Electronics housing |

## System Operation

1. Initialize the camera and capture an image of the pool.
2. Collect five valid ultrasonic measurements, subject to a maximum of 15 attempts.
3. Apply median filtering and a calibration offset based on the sensor's mounting position.
4. Connect to Wi-Fi and transmit the measurement and image through Telegram.
5. Enter deep sleep for 15 minutes before repeating the cycle.

The firmware also incorporates camera initialization retries, sensor timeouts, Wi-Fi connection timeouts, and camera frame buffer management to improve system reliability.

## Hardware Assembly

The circuit was assembled on a soldered perfboard using removable connections for the ESP32-S3-CAM and ultrasonic sensor module, allowing for easier maintenance, replacement, and troubleshooting.

| Assembled | Disassembled | Solder Connections |
|---|---|---|
| ![Assembled](images/perfboard-assembled.jpeg) | ![Disassembled](images/perfboard-disassembled.jpeg) | ![Bottom](images/perfboard-bottom.jpeg) |

## Enclosure Design

A custom two-piece enclosure was designed in Autodesk Fusion to house the electronic components.

| Assembled Enclosure | Base | Lid |
|---|---|---|
| ![Enclosure](images/enclosure_assembled.png) | ![Base](images/enclosure_base.png) | ![Lid](images/enclosure_lid.png) |

The original Fusion design and printable STL files are available in the [cad](cad/) directory.

## System Demonstration

| ESP32-S3-CAM Image Capture | Telegram Notification |
|---|---|
| ![Pool capture](images/pool_capture.jpeg) | ![Telegram notification](images/telegram-demo.jpeg) |

## Software Setup

**Requirements:**
- Arduino IDE
- ESP32 Arduino board package
- ArduinoJson
- UniversalTelegramBot

**Configuration:**

1. Download or clone the repository.
2. Open `firmware/firmware.ino` in Arduino IDE.
3. Copy `secrets.example.h` to `secrets.h` and enter your Wi-Fi and Telegram credentials.
4. Configure the ESP32-S3 board settings with PSRAM enabled.
5. Adjust the calibration offset for your sensor's mounting position.
6. Compile and upload the firmware.

## Future Improvements

- Improve camera image quality.
- Implement historical water level logging.
- Add configurable water loss alerts.
- Develop a custom PCB to replace the perfboard.

## Acknowledgments

Camera configuration and pin definitions were adapted from Espressif's ESP32 Arduino camera example.
