/*
 * Bike Sound Level — ESP32-C3 Super Mini + KY-040 rotary encoder
 *
 * BLE HID media remote for iPhone (AirPods-style controls):
 *   Rotate CW/CCW     → volume up / down
 *   1× press encoder  → play / pause
 *   2× press encoder  → next track
 *   3× press encoder  → previous track
 *
 * Wiring (KY-040 → ESP32-C3):
 *   SW  (button) → GPIO 5
 *   DT  (data B) → GPIO 6
 *   CLK (data A) → GPIO 7
 *   +   → 3.3 V
 *   GND → GND
 *
 * Pair on iPhone: Settings → Bluetooth → "Bike Volume"
 */

#include <Arduino.h>
#include <HijelHID_BLEKeyboard.h>
  
// --- pins ---
static const uint8_t PIN_SW = 5;
static const uint8_t PIN_DT = 6;
static const uint8_t PIN_CLK = 7;

// --- timing ---
static const uint32_t DEBOUNCE_MS = 50;
static const uint32_t MULTI_CLICK_GAP_MS = 400;

// KY-040: 4 quadrature transitions per detent (one physical click)
static const int8_t DETENT_TICKS = 4;

HijelHID_BLEKeyboard keyboard("Bike Volume", "Bike-SoundLevel", 100);

volatile int32_t encoderRaw = 0;

// Valid Gray-code transitions only; invalid/bounce steps add 0
static const int8_t ENCODER_TABLE[] = {
    0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

void IRAM_ATTR onEncoderChange() {
  static uint8_t lastState = 0;
  const uint8_t state =
      (digitalRead(PIN_CLK) << 1) | digitalRead(PIN_DT);
  if (state == lastState) {
    return;
  }
  encoderRaw += ENCODER_TABLE[(lastState << 2) | state];
  lastState = state;
}

void sendVolume(int8_t direction) {
  if (!keyboard.isPaired()) {
    return;
  }
  if (direction > 0) {
    keyboard.tap(MEDIA_VOLUME_UP);
  } else if (direction < 0) {
    keyboard.tap(MEDIA_VOLUME_DOWN);
  }
}

void handleButtonClicks(uint8_t clicks) {
  if (!keyboard.isPaired()) {
    return;
  }
  switch (clicks) {
    case 1:
      keyboard.tap(MEDIA_PLAY_PAUSE);
      Serial.println("Play / pause");
      break;
    case 2:
      keyboard.tap(MEDIA_NEXT_TRACK);
      Serial.println("Next track");
      break;
    default:
      keyboard.tap(MEDIA_PREV_TRACK);
      Serial.println("Previous track");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Bike Volume — starting BLE HID");

  pinMode(PIN_SW, INPUT_PULLUP);
  pinMode(PIN_DT, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_CLK), onEncoderChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_DT), onEncoderChange, CHANGE);

  keyboard.begin();
  Serial.println("Waiting for iPhone connection…");
}

void loop() {
  static int32_t lastHandledRaw = 0;
  static bool buttonStable = HIGH;
  static bool buttonLastRead = HIGH;
  static uint32_t buttonChangeMs = 0;
  static uint8_t clickCount = 0;
  static uint32_t lastClickMs = 0;

  // --- encoder → one volume step per detent ---
  int32_t raw = 0;
  noInterrupts();
  raw = encoderRaw;
  interrupts();

  const int32_t delta = raw - lastHandledRaw;
  if (delta >= DETENT_TICKS) {
    sendVolume(1);
    lastHandledRaw += DETENT_TICKS;
  } else if (delta <= -DETENT_TICKS) {
    sendVolume(-1);
    lastHandledRaw -= DETENT_TICKS;
  }

  // --- button → multi-click (count on release, active LOW) ---
  bool reading = digitalRead(PIN_SW);
  if (reading != buttonLastRead) {
    buttonChangeMs = millis();
  }
  buttonLastRead = reading;

  if (millis() - buttonChangeMs >= DEBOUNCE_MS && reading != buttonStable) {
    const bool wasPressed = (buttonStable == LOW);
    buttonStable = reading;

    if (wasPressed && buttonStable == HIGH) {
      clickCount = (clickCount < 3) ? clickCount + 1 : 3;
      lastClickMs = millis();
    }
  }

  if (clickCount > 0 && millis() - lastClickMs >= MULTI_CLICK_GAP_MS) {
    handleButtonClicks(clickCount);
    clickCount = 0;
  }

  static bool wasPaired = false;
  bool paired = keyboard.isPaired();
  if (paired != wasPaired) {
    wasPaired = paired;
    Serial.println(paired ? "iPhone connected" : "iPhone disconnected");
  }

  delay(2);
}
