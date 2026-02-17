#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BleComboMouse.h>
#include <BleComboKeyboard.h>

// --- CONFIGURATION ---
#define DEBUG_MODE      true
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   32
#define OLED_RESET      -1 

// --- BUTTON PINS ---
#define BUTTON_PAUSE    5   
#define BUTTON_TRIGGER  27  

// --- TIMING SETTINGS ---
#define RESTART_TIMEOUT 60000 // Restart after 60s of disconnection

// Stealth & ID Settings
#define DEVICE_NAME     "K400 Plus" 
#define DEVICE_MANUF    "Logitech"
#define BATTERY_LEVEL   84

// --- GLOBAL OBJECTS ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
BleComboKeyboard bleKeyboard(DEVICE_NAME, DEVICE_MANUF, BATTERY_LEVEL);
BleComboMouse bleMouse(&bleKeyboard);

// --- STATE TRACKING ---
unsigned long lastActionTime = 0;
unsigned long nextActionDelay = 5000;
bool isPaused = false;
unsigned long pauseEndTime = 0;
String lastActionName = "None";

// New: Track how long we've been disconnected
unsigned long disconnectStartTime = 0; 

void setup() {
  if (DEBUG_MODE) {
    Serial.begin(115200);
    Serial.println("--- JIGGLER STARTING ---");
  }

  setCpuFrequencyMhz(80);

  // Initialize Screen
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    if (DEBUG_MODE) Serial.println("Screen Failed!");
    for(;;); 
  }
  
  display.setRotation(2); 
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();

  bleKeyboard.begin();
  bleMouse.begin();
  
  pinMode(BUTTON_PAUSE, INPUT_PULLUP);
  pinMode(BUTTON_TRIGGER, INPUT_PULLUP);
  
  randomSeed(analogRead(34));
}

void updateScreen(String topStatus, String bottomAction) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(topStatus);
  display.setCursor(0, 16);
  display.print("> " + bottomAction);
  display.display();
}

void performAction() {
  if (!bleKeyboard.isConnected()) return;

  int choice = random(0, 100);
  
  if (choice < 70) {
    int xMove = random(-2, 3);
    int yMove = random(-2, 3);
    bleMouse.move(xMove, yMove);
    delay(100); 
    bleMouse.move(-xMove, -yMove); 
    lastActionName = "Micro-Jiggle";
  } 
  else if (choice < 90) {
    bleKeyboard.write(KEY_F15);
    lastActionName = "Pressed F15";
  }
  else {
    bleMouse.move(0, 0, random(-1, 2)); 
    lastActionName = "Mouse Scroll";
  }

  if (DEBUG_MODE) {
    Serial.print("[ACTION] ");
    Serial.println(lastActionName);
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // --- BUTTON 1: PAUSE TOGGLE ---
  if (digitalRead(BUTTON_PAUSE) == LOW) {
    delay(50); 
    if (digitalRead(BUTTON_PAUSE) == LOW) {
      isPaused = !isPaused; 
      
      if (isPaused) {
        pauseEndTime = currentMillis + 36000000; 
        updateScreen("STATUS: STANDBY", "Manual Pause");
      } else {
        pauseEndTime = 0; 
        lastActionTime = currentMillis; 
        nextActionDelay = 2000; 
        updateScreen("STATUS: ACTIVE", "Resuming...");
      }
      while(digitalRead(BUTTON_PAUSE) == LOW) { delay(10); } 
    }
  }

  // --- BUTTON 2: FORCE TRIGGER ---
  if (digitalRead(BUTTON_TRIGGER) == LOW && !isPaused) {
     delay(50);
     if (digitalRead(BUTTON_TRIGGER) == LOW) {
       updateScreen("STATUS: FORCED", "Triggering...");
       performAction();
       lastActionTime = currentMillis; 
       while(digitalRead(BUTTON_TRIGGER) == LOW) { delay(10); }
     }
  }

  // --- CONNECTION CHECK & AUTO-RESTART ---
  if (!bleKeyboard.isConnected()) {
    // 1. Start the disconnection timer if not already started
    if (disconnectStartTime == 0) {
      disconnectStartTime = currentMillis;
    }
    
    // 2. Calculate time waiting
    unsigned long timeDisconnected = currentMillis - disconnectStartTime;
    long timeToRestart = (RESTART_TIMEOUT - timeDisconnected) / 1000;
    
    // 3. Update Screen with Countdown
    if (timeToRestart > 0) {
      if (currentMillis % 1000 < 100) {
        updateScreen("STATUS: WAITING", "Reset in " + String(timeToRestart) + "s");
        if (DEBUG_MODE) Serial.println("Waiting for connection...");
      }
    } else {
      // 4. TIMEOUT REACHED -> REBOOT
      updateScreen("STATUS: REBOOT", "Fixing Bluetooth...");
      delay(1000);
      if (DEBUG_MODE) Serial.println("Rebooting to fix connection...");
      ESP.restart(); // <--- The Magic Fix
    }
    
    delay(100);
    return;
  } else {
    // We are connected, reset the timer
    disconnectStartTime = 0; 
  }

  // --- PAUSE LOGIC ---
  if (isPaused) {
    if (currentMillis > pauseEndTime) {
      isPaused = false;
      lastActionTime = currentMillis; 
    } else {
      long remaining = (pauseEndTime - currentMillis) / 1000;
      if (remaining > 3600) {
         if (currentMillis % 2000 == 0) updateScreen("STATUS: STANDBY", "Manual Pause");
      } else {
         if (currentMillis % 1000 < 50) updateScreen("STATUS: PAUSED", "Break: " + String(remaining) + "s");
      }
      return; 
    }
  }

  // --- TIMER LOGIC ---
  if (currentMillis - lastActionTime > nextActionDelay) {
    performAction();
    lastActionTime = currentMillis;
    
    nextActionDelay = random(30000, 150000); 

    if (random(0, 100) < 5) {
      isPaused = true;
      long breakDuration = random(60000, 120000); 
      pauseEndTime = currentMillis + breakDuration;
    }
  }

  // --- UI UPDATE ---
  static unsigned long lastScreenUpdate = 0;
  if (currentMillis - lastScreenUpdate > 1000) {
    long timeLeft = (nextActionDelay - (currentMillis - lastActionTime)) / 1000;
    if (timeLeft < 0) timeLeft = 0; 
    updateScreen("Next: " + String(timeLeft) + "s", lastActionName);
    lastScreenUpdate = currentMillis;
  }
}
