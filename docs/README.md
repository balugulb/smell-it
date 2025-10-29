# 📘 smell-It Documentation

This folder contains technical documentation, component descriptions, and references for the **smell-It** project.  
The main goal of this project is to provide a modular and maintainable firmware structure for an ESP32-based smell sensing and visualization system.

---

## 📂 Project Overview

**smell-It** is built on the ESP-IDF framework and organized into independent components for better maintainability and scalability.

### Core Features
- **Wi-Fi SoftAP** for local wireless communication  
- **TCP Server** for data exchange with external devices  
- **TFT Display** for real-time visualization  
- **Deep Sleep Management** for power saving 
- **Touch Interface** for wakeup purpose (subject to change)

---

## 🧩 Components

| Component | Description | Source / Documentation |
|------------|--------------|------------------------|
| **adafruit_tft** | Library for the 1.8" TFT Display ST7335 | [View on GitHub ›](https://github.com/adafruit/Adafruit-ST7735-Library.git) |
| **arduino** | Library for use of Arduino functions within ESP-IDF | [View on GitHub ›](https://github.com/espressif/arduino-esp32.git) |
| **adafruit_busio** | Library used by adafruit_tft | [View on GitHub ›](https://github.com/adafruit/Adafruit_BusIO.git) |
| **adafruit_gfx** | Library used by adafruit_tft | [View on GitHub ›](https://github.com/adafruit/Adafruit-GFX-Library.git) |

---

## ⚙️ Build Information

This project is based on:
- **ESP-IDF:** v5.3.2  
- **PlatformIO:** ESP32 platform  
- **Language:** C/C++ (with `extern "C"` interop for ESP-IDF)  
- **Target:** ESP32esp32

**ESP-IDF Website:** [here] https://docs.espressif.com/projects/esp-idf/en/stable/esp32/index.html
