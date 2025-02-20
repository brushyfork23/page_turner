# Page Turner

This repository contains code and models I used to build a touchless gesture-sensing bluetooth page turner. This device lets the user wave their hand over a gesture sensor to turn the pages of an ebook on an e-reader like an iPad.

## Hardware

The code ([page_turner.ino](page_turner.ino)) is uploaded to an [Adafruit QT Py S3]([https://www.adafruit.com/product/5700). The QT Py is connected via a [JST SH cable](https://www.adafruit.com/product/4399) to an [Adafruit APDS9960](https://www.adafruit.com/product/3595). An additional wire is soldered between the APDS9960's INT pin and the QT Py's A3 pin. On the backside of the QT Py is soldered an [Adafruit LiPoly Charger BFF](https://www.adafruit.com/product/5397). A [400mAh battery](https://www.adafruit.com/product/3898) is plugged into the charger.

## Libraries

-   [Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel/tree/f01f618b0671fc8a5437eb6e47184bea74f1be60)
-   [T-vK/ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard/tree/b7aaf9bb711a04216e4417f1e2a6b0ee0eaeaf66)
-   [Dgemily/APDS-9960_Gesture_Sensor_esp8266_Library](https://github.com/Dgemily/APDS-9960_Gesture_Sensor_esp8266_Library/tree/ace7afaba07e7df73d7fd7199d35698608a5d69b)  
    I used a fork of SparkFun's APDS-9960 sensor library because the SparkFun library directly calls Wire.begin() but I'm using the QT Py's JST SH port which does not use the default Wire pins. I also like Dgemily's suggestion of using interrupt callback functions in IRAM.
