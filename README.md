# ESP32-WROOM Morphing Clock (ESP-IDF)

A flicker-free implementation of the famous Morphing Clock, ported to **ESP-IDF** and based on the **SmartMatrix** library.

This project is a specialized version of the original Morphing Clock by [bogd](https://github.com/bogd/esp32-morphing-clock), optimized for ESP-IDF and HUB75 LED matrices.

![IMG_1781-ezgif com-crop](https://github.com/user-attachments/assets/3feb10c1-8f69-4a5d-8cad-b75926795405)


## 🚀 Features
- **Morphing Digits:** Smooth, high-refresh animations between time changes.
- **Weather Integration:** Real-time data and forecasts via **OpenMeteo** (No subscription/API key required).
- **Daily Advice:** "Tip of the Day" function triggered daily at 19:00.
- **Hidden Easter Egg:** Accessible via the web interface by clicking the SSID 5 times.
- **System Health:** Active memory tracking and safety restarts during night hours to prevent fragmentation.
- **Auto-Dimming:** Reduces luminescence at night for comfortable viewing.
- **OTA Updates:** Wireless firmware updates via a web page—no cables required.
- **MQTT Support:** Display custom messages on the status line (Enable in `config.h`).

## 🛠 Prerequisites

### Hardware
- **Controller:** ESP32-WROOM
- **Display:** HUB75 LED Matrix (64x32) (https://www.aliexpress.com/item/1005004775953349.html?spm=a2g0o.order_list.order_list_main.5.3c6f1802yUjd2G)
- **Power:** External 5V Power Supply (https://www.aliexpress.com/item/1005008680369173.html?spm=a2g0n.order_detail.order_detail_item.3.7b5bf19cvp1ZEB)
- **Case:** Optional/Custom (https://www.etsy.com/listing/1672811469/rgb-led-matrix-enclosure-diy-time?variation1=6416399426)

### Software & Environment
To ensure a problem-free compilation, please match these versions:
- **ESP-IDF:** v5.2.x (or compatible); Espressif IDF extention v.1.7.1
- **Python:** v3.11.2

## 📌 Pinout Configuration
Precompiled `.bin` file is available in the root directory. But! Ensure your hardware matches this mapping:


| Signal | GPIO | Signal | GPIO |
| :--- | :--- | :--- | :--- |
| **R1** | 27 | **G1** | 26 |
| **B1** | 14 | **R2** | 12 |
| **G2** | 25 | **B2** | 15 |
| **A** | 32 | **B** | 17 |
| **C** | 33 | **D** | 16 |
| **E** | 5 | **LAT** | 2 |
| **OE** | 4 | **CLK** | 0 |

> **Note:** To change these pins, edit `components/SmartMatrix/MatrixHardware_ESP32_V0.h` under the `ESP32_FORUM_PINOUT` section.

## 📜 License & Credits

This project is an evolution of the **ESP32 HUB75 Matrix Morphing Clock** originally created by **Bogdan Sass** ([@bogd](https://github.com)). 

In the spirit of open-source collaboration and in compliance with the original licensing:
- This project is licensed under the **GNU General Public License v3.0**.
- You are free to use, modify, and distribute this code, provided that all derivatives remain open-source under the same GPLv3 terms.

A huge thank you to **Bogdan Sass** for the foundational work and the inspiration to port this to ESP-IDF!
