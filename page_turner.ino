#define APDS9960_INT 8
#define APDS9960_SDA 41
#define APDS9960_SCL 40
#define PROX_THRESHOLD 10
#define BATTERY_PIN A2
#define BATTERY_SAMPLES 16
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

void println(float num) {
  #ifdef DEBUG_PRINT
  Serial.print(num);
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

float batteryVoltage = 0;

float readBatteryVoltage() {
    uint32_t millivolts = 0;
    
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
        millivolts += analogReadMilliVolts(BATTERY_PIN);
        delayMicroseconds(100);
    }
    
    float avgMillivolts = millivolts / (float)BATTERY_SAMPLES;
    batteryVoltage = (avgMillivolts / 1000.0) * 2.0;
    print(F("Battery voltage: "));
    println(batteryVoltage);
    return batteryVoltage;
}

uint32_t getBatteryStatusColor() {
    print(F("Battery status: "));
    if (batteryVoltage > 4.0) {
      println(F("Charging/USB"));
      return pixels.Color(0, 255, 0);
    } else if (batteryVoltage > 3.7) {
      println(F("Good"));
      return pixels.Color(0, 255, 0);
    } else if (batteryVoltage > 3.6) {
      println(F("Low"));
      return pixels.Color(255, 200, 0);
    } else {
      println(F("Critical"));
      return pixels.Color(255, 0, 0);
    }
}

void setup() {
  #ifdef DEBUG_PRINT
  Serial.begin(115200);
  delay(50);
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

  // Configure ADC settings for optimal battery monitoring
  analogSetAttenuation(ADC_11db);    // 0-3100mV range
  analogReadResolution(12);          // 12-bit resolution
  readBatteryVoltage();
  pixels.setPixelColor(0, getBatteryStatusColor());
  pixels.show();
  delay(1000);
  pixels.setPixelColor(0,0);
  pixels.show();

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
    if (currentTime - lastBlinkTime >= 2000) {
      lastBlinkTime = currentTime;
      pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      pixels.show();
      delay(20);
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