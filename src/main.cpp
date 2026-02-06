#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BleMouse.h>
#include <BleKeyboard.h>

// --- CONFIGURATION ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 
#define BUTTON_PIN    0   // The BOOT button on most ESP32s

// Stealth & ID Settings (Logitech MX Master 3 Spoof)
#define DEVICE_NAME   "MX Master 3" 
#define DEVICE_MANUF  "Logitech"
#define HID_VID       0x046D
#define HID_PID       0xB023

// --- GLOBAL OBJECTS ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
BleMouse bleMouse(DEVICE_NAME, DEVICE_MANUF, 100);
BleKeyboard bleKeyboard(DEVICE_NAME, DEVICE_MANUF, 100);

// --- STATE TRACKING ---
unsigned long lastActionTime = 0;
unsigned long nextActionDelay = 5000; // Starts at 5 seconds
bool isPaused = false;
unsigned long pauseEndTime = 0;

void setup() {
  // 1. Lower CPU speed to 80MHz for battery saving
  setCpuFrequencyMhz(80);

  // 2. Setup Screen
  // Address 0x3C is standard for these generic OLEDs
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    for(;;); // Don't proceed, loop forever if screen fails
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Initializing...");
  display.println("Stealth Mode: ON");
  display.display();

  // 3. Setup Bluetooth with Spoofed ID
  // Note: Library handling of VID/PID happens internally or via BLEDevice config
  // For standard BleMouse/Keyboard libs, we set the name which is the most visible part.
  bleMouse.begin();
  bleKeyboard.begin();
  
  // 4. Setup Button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Random seed from unconnected pin noise
  randomSeed(analogRead(34));
}

void updateScreen(String status, String action) {
  display.clearDisplay();
  
  // Screensaver logic: Randomize text position slightly to prevent burn-in
  int x = random(0, 10);
  int y = random(0, 20);
  
  display.setCursor(x, y);
  display.setTextSize(1);
  display.println("Running: " + String(DEVICE_NAME));
  
  display.setCursor(x, y + 15);
  display.println("Status: " + status);
  
  display.setCursor(x, y + 30);
  display.println("Last: " + action);

  // Battery simulation (Visual only)
  display.setCursor(x, y + 45);
  display.println("Battery: Optimal");
  
  display.display();
}

void performAction() {
  if (!bleMouse.isConnected()) return;

  int choice = random(0, 100);
  String actionName = "";

  if (choice < 70) {
    // 70% Chance: Micro Mouse Jiggle
    // Move slightly, wait briefly, move back (Zero-sum movement)
    int xMove = random(-2, 3); // -2 to +2
    int yMove = random(-2, 3);
    
    bleMouse.move(xMove, yMove);
    delay(random(50, 150)); // Tiny human-like micro-pause
    bleMouse.move(-xMove, -yMove); // Return to start
    
    actionName = "Mouse Micro-Move";
  } 
  else if (choice < 90) {
    // 20% Chance: Ghost Key Press (F15)
    // F15 is rarely used by apps but registers as activity
    bleKeyboard.write(KEY_F15);
    actionName = "Key: F15";
  }
  else {
    // 10% Chance: Mouse Scroll
    // Scrolling 1 unit is very subtle
    bleMouse.move(0, 0, random(-1, 2)); 
    actionName = "Mouse Scroll";
  }

  updateScreen("Active", actionName);
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Connection Check
  if (!bleMouse.isConnected()) {
    updateScreen("Disconnected", "Waiting for PC...");
    delay(1000);
    return;
  }

  // 2. Pause Logic (Simulating "Thinking" or Reading)
  if (isPaused) {
    if (currentMillis > pauseEndTime) {
      isPaused = false;
      updateScreen("Resuming...", "Work Mode");
    } else {
      // Just update screen occasionally during pause
      if (currentMillis % 5000 < 100) updateScreen("Idling...", "Human Break");
      return; 
    }
  }

  // 3. Trigger Action
  if (currentMillis - lastActionTime > nextActionDelay) {
    performAction();
    lastActionTime = currentMillis;
    
    // Set next delay: Random between 10 seconds and 3 minutes
    // This irregularity makes it harder to detect than a fixed "every 60s" loop.
    nextActionDelay = random(10000, 180000); 

    // 4. Randomly decide to take a "Break" (simulated idle time)
    // 5% chance after every action to pause for 1-5 minutes
    if (random(0, 100) < 5) {
      isPaused = true;
      long breakDuration = random(60000, 300000); // 1 to 5 minutes
      pauseEndTime = currentMillis + breakDuration;
      updateScreen("Taking Break", String(breakDuration/1000) + "s");
    }
  }
}
