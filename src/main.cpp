#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BleComboMouse.h>
#include <BleComboKeyboard.h>

// --- CONFIGURATION ---
#define DEBUG_MODE    true
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1 

// --- BUTTON PINS ---
#define BUTTON_PAUSE   5   // Toggle Standby
#define BUTTON_TRIGGER 27  // Force Immediate Action

// Stealth & ID Settings
#define DEVICE_NAME   "K400 Plus" 
#define DEVICE_MANUF  "Logitech"
#define BATTERY_LEVEL 84

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
  
  // Initialize Physical Buttons
  // INPUT_PULLUP means you wire the button between the PIN and GROUND.
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

  // --- BUTTON 1: PAUSE TOGGLE (Pin 5) ---
  if (digitalRead(BUTTON_PAUSE) == LOW) {
    delay(50); // Debounce
    if (digitalRead(BUTTON_PAUSE) == LOW) {
      isPaused = !isPaused; 
      
      if (isPaused) {
        // Manual Pause (Indefinite)
        pauseEndTime = currentMillis + 36000000; 
        updateScreen("STATUS: STANDBY", "Manual Pause");
        if (DEBUG_MODE) Serial.println("[MANUAL] User paused device.");
      } else {
        // Resume
        pauseEndTime = 0; 
        lastActionTime = currentMillis; 
        nextActionDelay = 2000; // Act quickly after resuming
        updateScreen("STATUS: ACTIVE", "Resuming...");
        if (DEBUG_MODE) Serial.println("[MANUAL] User resumed device.");
      }
      while(digitalRead(BUTTON_PAUSE) == LOW) { delay(10); } 
    }
  }

  // --- BUTTON 2: FORCE TRIGGER (Pin 27) ---
  if (digitalRead(BUTTON_TRIGGER) == LOW && !isPaused) {
     delay(50);
     if (digitalRead(BUTTON_TRIGGER) == LOW) {
       updateScreen("STATUS: FORCED", "Triggering...");
       performAction();
       lastActionTime = currentMillis; // Reset timer
       // Wait a moment so we don't spam 50 clicks
       while(digitalRead(BUTTON_TRIGGER) == LOW) { delay(10); }
     }
  }

  // --- CONNECTION CHECK ---
  if (!bleKeyboard.isConnected()) {
    if (currentMillis % 2000 < 1000) updateScreen("STATUS: WAITING", "Pairing...");
    else updateScreen("STATUS: WAITING", ""); 
    delay(100);
    return;
  }

  // --- PAUSE LOGIC ---
  if (isPaused) {
    if (currentMillis > pauseEndTime) {
      isPaused = false;
      lastActionTime = currentMillis; 
    } else {
      // If manually paused, just show STANDBY
      // If auto-break paused, show countdown
      long remaining = (pauseEndTime - currentMillis) / 1000;
      if (remaining > 3600) {
         // It's a manual pause (huge number)
         if (currentMillis % 2000 == 0) updateScreen("STATUS: STANDBY", "Manual Pause");
      } else {
         // It's a random break
         if (currentMillis % 1000 < 50) updateScreen("STATUS: PAUSED", "Break: " + String(remaining) + "s");
      }
      return; 
    }
  }

  // --- TIMER LOGIC ---
  if (currentMillis - lastActionTime > nextActionDelay) {
    performAction();
    lastActionTime = currentMillis;
    
    // SAFE TIMING: 30s to 2.5 mins (Beats 5 min lock)
    nextActionDelay = random(30000, 150000); 

    // 5% Chance of short Auto-Break (1-2 mins)
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
