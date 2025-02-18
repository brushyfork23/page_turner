#define APDS9960_INT 8
#define APDS9960_SDA 41
#define APDS9960_SCL 40
// #define DEBUG_PRINT
// #define DEBUG_GESTURE

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

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

#include <Wire.h>
#include <SparkFun_APDS9960.h>

SparkFun_APDS9960 apds = SparkFun_APDS9960();
volatile bool isr_flag = 0;

void ICACHE_RAM_ATTR interruptRoutine ();

bool sensor_enabled = false;

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

  if ( apds.init() ) {
    println(F("APDS-9960 initialization complete"));
  } else {
    println(F("Something went wrong during APDS-9960 init!"));
  }

  bleKeyboard.begin();
}

void loop() {
  #ifdef DEBUG_GESTURE
  bool debug_gesture = true;
  #else
  bool debug_gesture = false;
  #endif

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
      startInterrupt();
      if (!apds.enableGestureSensor(true)) {
        println(F("Something went wrong during gesture sensor init!"));
      } else {
        println(F("Gesture sensor initialized"));
      }
    }

  } else {
    if (sensor_enabled) {
      sensor_enabled = false;
      stopInterrupt();
      apds.disablePower();
      println(F("Sensor disabled"));
    }

    static unsigned long lastBlinkTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= 5000) {
      lastBlinkTime = currentTime;
      pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      pixels.show();
      delay(10);
      pixels.setPixelColor(0, 0);
      pixels.show();
    }
  }

  if( isr_flag == 1 ) {
    stopInterrupt();
    handleGesture();
    isr_flag = 0;
    startInterrupt();
  }
}

void startInterrupt() {
  attachInterrupt(APDS9960_INT, interruptRoutine, FALLING);
}

void stopInterrupt() {
  detachInterrupt(APDS9960_INT);
}

void handleGesture() {
  if ( apds.isGestureAvailable() ) {
    print(F("Gesture: "));
    switch ( apds.readGesture() ) {
      case DIR_UP:
        println(F("UP"));
        break;
      case DIR_DOWN:
        println(F("DOWN"));
        break;
      case DIR_LEFT:
        println(F("LEFT"));
        pageBack();
        break;
      case DIR_RIGHT:
        println(F("RIGHT"));
        pageForward();
        break;
      case DIR_NEAR:
        println(F("NEAR"));
        break;
      case DIR_FAR:
        println(F("FAR"));
        break;
      default:
        println(F("NONE"));
    }
  }
}

void pageBack() {
  if (bleKeyboard.isConnected()) {
    bleKeyboard.write(KEY_LEFT_ARROW);
  }
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show();
  delay(10);
  pixels.setPixelColor(0, 0);
  pixels.show();
}

void pageForward() {
  if (bleKeyboard.isConnected()) {
    bleKeyboard.write(KEY_RIGHT_ARROW);
  }
  pixels.setPixelColor(0, pixels.Color(0, 0, 255));
  pixels.show();
  delay(10);
  pixels.setPixelColor(0, 0);
  pixels.show();
}

void interruptRoutine() {
  isr_flag = 1;
}