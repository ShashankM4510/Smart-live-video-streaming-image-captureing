# Telegram-controlled camera with GPS tracking and relay control  
**ESP32 Smart Surveillance System**

![Project Banner](https://via.placeholder.com/800x300?text=ESP32+Smart+Surveillance+System)  
*Replace with your project image*

---

## 📋 Table of Contents
1. [Project Overview](#-project-overview)
2. [Features](#-features)
3. [Hardware Requirements](#-hardware-requirements)
4. [Pin Configuration](#-pin-configuration)
5. [Setup Instructions](#-setup-instructions)
6. [Telegram Bot Commands](#-telegram-bot-commands)
7. [Web Interface](#-web-interface)
8. [Troubleshooting](#-troubleshooting)
9. [Security Notes](#-security-notes)
10. [License](#-license)

---

## 🚀 Project Overview
This ESP32-based system provides:
- 📷 Remote camera control via Telegram
- 🗺️ GPS location tracking
- 🔌 Relay control for external devices
- 🌐 Live video streaming
- 💡 Flashlight control

---

## ✨ Features
| Feature | Description |
|---------|-------------|
| **Telegram Integration** | Control all functions via Telegram bot |
| **OV2640 Camera** | Capture and send photos (VGA resolution) |
| **GPS Tracking** | NEO-6M module for location data |
| **Web Streaming** | MJPEG live stream accessible via browser |
| **Relay Control** | Remote control of electrical devices |
| **Flash Control** | Built-in LED flashlight |

---

## 🔧 Hardware Requirements
### Core Components
- ESP32 Development Board
- OV2640 Camera Module
- NEO-6M GPS Module
- 5V Relay Module
- 3.7V LiPo Battery (or 5V power supply)

### Optional Components
- FTDI Programmer (if board lacks USB)
- External LED for flash

---

## 🔌 Pin Configuration
| ESP32 Pin | Connected To |
|-----------|--------------|
| GPIO12    | Relay Control |
| GPIO4     | Flash LED |
| GPIO13    | GPS RX |
| GPIO14    | GPS TX |
| GPIO32    | Camera PWDN |
| GPIO0     | Camera XCLK |
| GPIO26/27 | Camera I2C |
| GPIO35-39 | Camera Data |

---

## 🛠️ Setup Instructions

### 1. Software Requirements
```arduino
Required Libraries:
- UniversalTelegramBot
- ArduinoJson
- TinyGPS++
- WebServer
- WiFiClientSecure
- esp_camera
