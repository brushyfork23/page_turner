#define APDS9960_INT 8
#define APDS9960_SDA 41
#define APDS9960_SCL 40
#define PROX_INT_HIGH 150
#define PROX_INT_LOW 0
// #define DEBUG_PRINT
// When GESTURE_SENSING is defined, left and right gestures
// will send left and right arrow key commands. When not
// defined, proximity detection will be used instead and
// only right arrow key commands will be sent.
// #define GESTURE_SENSING
//  #define DEBUG_SENSOR

#include <Arduino.h>

unsigned long lastWriteTime = 0;

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
#include <SparkFun_APDS9960.h>

SparkFun_APDS9960 apds = SparkFun_APDS9960();
uint8_t proximity_data = 0;
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

  #ifndef GESTURE_SENSING
  // Adjust the Proximity sensor gain
  if ( !apds.setProximityGain(PGAIN_2X) ) {
    println(F("Something went wrong trying to set PGAIN"));
  }
  // Set proximity interrupt thresholds
  if ( !apds.setProximityIntLowThreshold(PROX_INT_LOW) ) {
    println(F("Error writing low threshold"));
  }
  if ( !apds.setProximityIntHighThreshold(PROX_INT_HIGH) ) {
    println(F("Error writing high threshold"));
  }
  #endif

  bleKeyboard.begin();
}

void loop() {
  #ifdef DEBUG_SENSOR
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
      digitalWrite(NEOPIXEL_POWER, LOW);
      startInterrupt();
      #ifdef GESTURE_SENSING
      if (!apds.enableGestureSensor(true)) {
        println(F("Something went wrong during gesture sensor init!"));
      } else {
        println(F("Gesture sensor initialized"));
      }
      #else
      if ( apds.enableProximitySensor(true) ) {
        println(F("Proximity sensor is now running"));
      } else {
        println(F("Something went wrong during sensor init!"));
      }
      #endif
    }

  } else {
    if (sensor_enabled) {
      sensor_enabled = false;
      stopInterrupt();
      apds.disablePower();
      println(F("Sensor disabled"));
      digitalWrite(NEOPIXEL_POWER, HIGH);
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
    #ifdef GESTURE_SENSING
    handleGesture();
    #else
    handleProximity();
    #endif
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

#ifdef GESTURE_SENSING
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
#else
void handleProximity() {
    if ( !apds.readProximity(proximity_data) ) {
      println(F("Error reading proximity value"));
    }
    
    static unsigned long lastDetectTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastDetectTime >= 50) {
      print(F("Proximity detected! "));
      println(proximity_data);
      pageForward();
    }
    lastDetectTime = currentTime;

    if ( !apds.clearProximityInt() ) {
      println(F("Error clearing interrupt"));
    }
}
#endif

#ifdef GESTURE_SENSING
void pageBack() {
  if (bleKeyboard.isConnected() && !recentlyWritten()) {
    bleKeyboard.write(KEY_LEFT_ARROW);
  }
  if (bleKeyboard.isConnected()) {
    bleKeyboard.write(KEY_LEFT_ARROW);
  }
}
#endif

void pageForward() {
  if (bleKeyboard.isConnected() && !recentlyWritten()) {
    bleKeyboard.write(KEY_RIGHT_ARROW);
  }
  if (bleKeyboard.isConnected()) {
    bleKeyboard.write(KEY_RIGHT_ARROW);
  }
}

bool recentlyWritten() {
  unsigned long currentTime = millis();
  if (currentTime - lastWriteTime >= 1500) {
    lastWriteTime = currentTime;
    return false;
  }
  return true;
}

void interruptRoutine() {
  isr_flag = 1;
}