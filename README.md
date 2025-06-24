# 🌫️ Air Pollution Monitoring System

An IoT-based real-time air quality monitoring system using ESP8266 NodeMCU and multiple gas sensors. The system reads, analyzes, and visualizes key environmental data—and alerts users when hazardous gas levels are detected.

---

## 📋 Project Summary

This project monitors air quality using various gas and environmental sensors connected to a NodeMCU microcontroller. It uploads real-time data to **ThingSpeak** and sends alerts via **Pushbullet** when gas levels exceed predefined safety thresholds.

---

## 🧠 Features

- 📡 **IoT Integration**: WiFi-enabled data transmission to ThingSpeak.
- 📈 **Real-time Monitoring**: Continuous tracking of gas concentrations and environmental conditions.
- 🚨 **Pushbullet Alerts**: Instant alerts for dangerous gas levels.
- 📊 **Data Visualization**: Dynamic dashboard on ThingSpeak.
- 🔧 **Modular Design**: Easily extensible for additional sensors or features.

---

## 🧰 Tech Stack

### 💻 Hardware
- ESP8266 NodeMCU (Microcontroller)
- Gas Sensors:
  - MQ135 (CO₂)
  - MQ2 (LPG)
  - MQ7 (CO)
  - MQ8 (Hydrogen)
  - MQ4 (Methane/CH₄)
- DHT11 (Temperature & Humidity Sensor)

### 🧑‍💻 Software
- Arduino IDE (C++)
- Libraries:
  - `ESP8266WiFi`
  - `ESP8266HTTPClient`
  - `WiFiClientSecure`
  - `ArduinoJson`
  - `DHT`
  - Sensor-specific libraries

---


