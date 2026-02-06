# Stealth ESP32 BLE Mouse Jiggler (OLED Edition)

A highly advanced, "undetectable" Bluetooth LE Mouse Jiggler based on the ESP32. This fork significantly upgrades the original project with **Hardware ID Spoofing**, **Human-Like Behavior Algorithms**, **OLED Dashboard**, and **Physical Control Buttons**.

Designed to prevent screen locks in strict IT environments (< 5min timeouts) while remaining invisible to standard behavioral analysis tools.

## 🚀 Key Features

### 🕵️ Stealth & Evasion
* **Hardware Spoofing:** Identifies itself as a **Logitech K400 Plus** (Keyboard/Trackpad Combo) rather than a generic "ESP32" device.
* **"Combo" Device Class:** Uses a single Bluetooth connection for both Mouse and Keyboard signals to avoid "Dual Device" flags in security logs.
* **Human-Like Activity:** Instead of robotic repetitive movements, this device randomizes between:
    * **Micro-Jiggles:** 1-pixel movement and immediate return (Zero-sum).
    * **Ghost Keys:** Presses `F15` (a harmless, invisible function key).
    * **Scroll Events:** Subtle mouse wheel ticks.
* **Smart Timing:** Actions occur at randomized intervals (30s to 2.5m) to defeat aggressive screen locks without creating a predictable "heartbeat" pattern.
* **Simulated Breaks:** Occasionally pauses for 1–2 minutes to mimic a human reading a document or taking a sip of coffee.

### 🖥️ Hardware & UI
* **OLED Dashboard (128x32):** Rotated 180° for optimal cable management.
    * **Top Line:** Countdown timer to next action (`Next: 42s`).
    * **Bottom Line:** Live log of the last action performed (`> Micro-Jiggle`).
* **Physical Controls:**
    * **Pause Switch:** Toggle "Standby Mode" when you want to use the mouse yourself.
    * **Trigger Button:** Force an immediate activity event if the screen is dimming.

### 🔋 Efficiency
* **Underclocked:** CPU restricted to **80MHz** (down from 240MHz) to significantly extend battery life if running untethered.

---

## 🛠️ Hardware Requirements

* **ESP32 Development Board** (ESP-WROOM-32 or similar)
* **0.91" OLED Display** (128x32 I2C)
* **2x Push Buttons** (Momentary)
* Wiring to pins `GPIO 5`, `GPIO 27`, `SDA`, `SCL`, `GND`, `3V3`.

### Wiring Diagram

| Component | ESP32 Pin | Note |
| :--- | :--- | :--- |
| **OLED SDA** | `GPIO 21` | Standard I2C SDA |
| **OLED SCL** | `GPIO 22` | Standard I2C SCL |
| **Button 1 (Pause)** | `GPIO 5` | Connect to GND (Internal Pullup) |
| **Button 2 (Trigger)** | `GPIO 27` | Connect to GND (Internal Pullup) |

---

## 💾 Installation

This project is built using **PlatformIO** (VS Code).

1.  **Clone the Repo:**
    ```bash
    git clone https://github.com/yourusername/esp32-mouse-jiggler.git
    ```
2.  **Open in VS Code:** Ensure the PlatformIO extension is installed.
3.  **Upload:** Connect your ESP32 via USB and click the "Upload" (Arrow) button.
    * *Note: The first build will automatically download the required libraries (`BleCombo`, `Adafruit GFX`, `SSD1306`).*

### Configuration (`main.cpp`)
You can tweak the behavior at the top of `src/main.cpp`:

```cpp
// Set to 'false' for maximum stealth (disables USB Serial logging)
#define DEBUG_MODE    true 

// Timing adjustments (in milliseconds)
// Current: Randomly acts between 30s and 2.5 minutes
nextActionDelay = random(30000, 150000);
```

## 🎮 Usage Guide

### The Buttons
* **Pause Button (Pin 5):**
    * **Press Once:** Enters **STANDBY** mode. The device stops all activity. Use this when you are working to prevent the cursor from fighting you.
    * **Press Again:** RESUMES activity immediately.
* **Trigger Button (Pin 27):**
    * **Press Once:** Forces the device to act *now*. Useful if you catch your screen dimming and don't want to wait for the timer.

### The Screen
* **STATUS: ACTIVE** - Device is running and protecting your session.
* **STATUS: STANDBY** - Device is manually paused (via button).
* **STATUS: PAUSED** - Device is taking a random "Human Break" (will resume automatically in <2 mins).
* **STATUS: WAITING** - Bluetooth disconnected. Waiting for PC.

---

## ⚠️ Disclaimer
This software is for educational purposes only. Using this device to bypass corporate IT security policies may violate your employment agreement. The author accepts no responsibility for any consequences resulting from the use of this code.

**Credits:**
Based on the original work by [monstermuffin](https://github.com/monstermuffin/esp32-mouse-jiggler) and the `ESP32-BLE-Combo` library.

