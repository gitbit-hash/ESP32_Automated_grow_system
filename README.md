# ESP32 Automated Grow System

An automated plant growth system based on the ESP32 microcontroller. This project manages light scheduling, monitors environmental conditions (Temperature, Humidity, Soil Moisture), and supports Over-The-Air (OTA) updates and remote monitoring via Telnet.

## Features

- **Automated Light Schedule**: 
  - Implementation of an 18/6 light cycle (ON at 8:00 AM, OFF at 2:00 AM the next day).
  - Uses a DS3231 RTC for precise timekeeping and redundancy.
  - Automatically handles state recovery after power loss.
- **Environmental Monitoring**:
  - **Air**: Temperature and Humidity tracking via BME280.
  - **Soil**: Moisture level detection using a capacitive soil moisture sensor.
- **Connectivity**:
  - **WiFi**: Connects to local network with a static IP configuration.
  - **OTA Updates**: Supports wireless firmware flashing via ArduinoOTA.
  - **Remote Logging**: Real-time status output via Telnet (using `TelnetSpy`).

## Hardware Requirements

- **Microcontroller**: ESP32 Development Board (e.g., DOIT ESP32 DEVKIT V1)
- **Timekeeping**: DS3231 RTC Module
- **Sensors**:
  - BME280 (Temperature, Humidity, Pressure)
  - Capacitive Soil Moisture Sensor
- **Actuators**:
  - Relay Module (for Grow Lights)
  - Relay Module / MOSFET (for Water Pump)
- **Power Supply**: Appropriate 5V/12V supply for ESP32 and peripherals.

## Pin Configuration

| Component | ESP32 Pin | Description |
|-----------|-----------|-------------|
| **I2C Bus** | 21 (SDA), 22 (SCL) | Connection for DS3231 & BME280 |
| **Relay (Light)** | GPIO 26 | Controls the grow light |
| **Relay (Pump)** | GPIO 16 | Controls the water pump |
| **RTC Interrupt** | GPIO 23 | DS3231 Alarm Interrupt |
| **Soil Sensor** | GPIO 34 | Analog input for moisture reading |
| **Soil Power** | GPIO 4 | Digital output to power sensor only when reading |

## Software & Libraries

This project is built using **PlatformIO**.

**Dependencies:**
- `yasheena/TelnetSpy`
- `adafruit/RTClib`
- `adafruit/Adafruit BME280 Library`
- Built-in libs: `WiFi`, `ESPmDNS`, `ArduinoOTA`, `Wire`, `WiFiUdp`

## Installation

1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   ```
2. **Open in PlatformIO**:
   Open the project folder in VS Code with the PlatformIO extension installed.
3. **Configure Settings**:
   Edit `src/main.cpp` to match your environment:
   - **WiFi Credentials**: Update `SSID` and `WIFIPWD`.
   - **Network Config**: Adjust `local_IP`, `gateway`, and `subnet` in `connectWiFi()`.
   - **Calibration**: Adjust `DRY_VALUE` and `WET_VALUE` based on your soil sensor calibration.
4. **Build and Upload**:
   - Connect your ESP32 via USB.
   - Run the "Upload" task in PlatformIO.
   - *Note*: For subsequent updates, you can use the OTA Upload protocol if the device is connected to WiFi.

## Usage

- **Monitoring**: Connect to the device IP via Telnet or monitor the Serial output (Baud: 115200) to view real-time logs.
- **Light Control**: The system automatically controls the relay based on the RTC time.
- **Sensors**: Readings are printed to the console every loop iteration (approx. 1 second).

## Future Improvements

- Implementation of automated watering logic based on soil moisture thresholds.
- Web Dashboard for easier monitoring and configuration.
- MQTT integration for Home Assistant support.
