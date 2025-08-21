#define APDS9960_INT 8
#define APDS9960_SDA 41
#define APDS9960_SCL 40
#define PROX_THRESHOLD 10
// #define DEBUG_PRINT
// #define DEBUG_SENSOR

#include <Arduino.h>

void print(const __FlashStringHelper *ifsh) {
  #ifdef DEBUG_PRINT
  Serial.print(ifsh);
  #endif
}

void println(const __FlashStringHelper *ifsh) {
  #ifdef DEBUG_PRINT
  Serial.println(ifsh);
  #endif
}

void println(uint8_t num) {
  #ifdef DEBUG_PRINT
  Serial.println(num);
  #endif
}

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

#include <Wire.h>
#include "Adafruit_APDS9960.h"

Adafruit_APDS9960 apds;

bool sensor_enabled = false;
unsigned long lastWriteTime = 0;

unsigned long forwardIndicatorOnTime = 0;
bool forwardIndicatorOn = false;

#include <BleKeyboard.h>
BleKeyboard bleKeyboard("Page Turner");

void setup() {
  #ifdef DEBUG_PRINT
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("-----------"));
  Serial.println(F("Page Turner"));
  Serial.println(F("-----------"));
  #endif

  #if defined(NEOPIXEL_POWER)
    pinMode(NEOPIXEL_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_POWER, HIGH);
  #endif
  pixels.begin();
  pixels.setBrightness(20);

  Wire.begin(APDS9960_SDA,APDS9960_SCL);

  pinMode(APDS9960_INT, INPUT_PULLUP);

  if ( apds.begin() ) {
    println(F("APDS-9960 initialization complete"));
  } else {
    println(F("Something went wrong during APDS-9960 init!"));
  }

  apds.setProxGain(APDS9960_PGAIN_2X);
  apds.enableProximity(true);
  apds.setProximityInterruptThreshold(0, PROX_THRESHOLD);
  apds.enableProximityInterrupt();

  bleKeyboard.begin();
}

void loop() {
  #ifdef DEBUG_SENSOR
  bool debug_gesture = true;
  #else
  bool debug_gesture = false;
  #endif


  unsigned long currentTime = millis();
  if (forwardIndicatorOn && currentTime - forwardIndicatorOnTime >= 400) {
    pixels.setPixelColor(0, 0);
    pixels.show();
    forwardIndicatorOn = false;
  }

  if (bleKeyboard.isConnected() || debug_gesture) {
    if (!sensor_enabled) {
      sensor_enabled = true;
      for (int i = 0; i < 5; i++) {
        pixels.setPixelColor(0, pixels.Color(0, 255, 0));
        pixels.show();
        delay(10);
        pixels.setPixelColor(0, 0);
        pixels.show();
        delay(110);
      }
      apds.enable(true);
      apds.clearInterrupt();
      println(F("Sensor enabled"));
    }

  } else {
    if (sensor_enabled) {
      sensor_enabled = false;
      apds.enable(false);
      println(F("Sensor disabled"));
    }

    static unsigned long lastBlinkTime = 0;
    if (currentTime - lastBlinkTime >= 5000) {
      lastBlinkTime = currentTime;
      pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      pixels.show();
      delay(10);
      pixels.setPixelColor(0, 0);
      pixels.show();
    }
  }

  if( sensor_enabled && !digitalRead(APDS9960_INT) ) {
    handleProximity();
    apds.clearInterrupt();
  }
}

void handleProximity() {
    static unsigned long lastDetectTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastDetectTime >= 50) {
      #ifdef DEBUG_PRINT
      uint8_t proximity = apds.readProximity();
      print(F("Proximity detected! "));
      println(proximity);
      #endif
      pageForward();
    }
    lastDetectTime = currentTime;
}

void pageForward() {
  // if (bleKeyboard.isConnected() && !recentlyWritten()) {
  //   bleKeyboard.write(KEY_RIGHT_ARROW);
  // }
  if (bleKeyboard.isConnected()) {
    bleKeyboard.write(0x20);
  }
  pixels.setPixelColor(0, pixels.Color(0, 125, 0));
  pixels.show();
  forwardIndicatorOnTime = millis();
  forwardIndicatorOn = true;
}

bool recentlyWritten() {
  unsigned long currentTime = millis();
  if (currentTime - lastWriteTime >= 1500) {
    lastWriteTime = currentTime;
    return false;
  }
  return true;
}