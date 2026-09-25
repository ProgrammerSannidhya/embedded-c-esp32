# ESP32 Embedded C Projects

This repository contains my Embedded C projects using the ESP32 and ESP-IDF.

The projects are developed without the Arduino framework to build practical understanding of GPIO, timing, peripherals, communication protocols and embedded software architecture.

## Projects

### 1. ESP32 LED Blinking

Basic ESP32 GPIO project using ESP-IDF.

**Concepts:**
- GPIO configuration
- Digital output
- GPIO driver APIs
- ESP-IDF project structure

[View Project](./blinking_an_led)

---

### 2. ESP32 Ultrasonic Distance Monitor

An ESP32-based distance monitoring system using an HC-SR04 ultrasonic sensor and a 128×64 I2C OLED display.

**Features:**
- HC-SR04 distance measurement
- I2C OLED communication
- Real-time distance display
- SAFE/DANGER status
- LED warning indicator
- Custom OLED bitmap font
- ESP-IDF GPIO and I2C drivers
- FreeRTOS timing

**Hardware:**
- ESP32
- HC-SR04
- 128×64 I2C OLED
- LED
- Resistors
- Breadboard

**Concepts:**
- GPIO
- I2C
- Microsecond timing
- Ultrasonic sensor interfacing
- OLED controller communication
- FreeRTOS
- Embedded C

[View Project](./distance_calculator)

---

## Development Approach

These projects are being developed using:

- Embedded C
- ESP-IDF
- FreeRTOS
- ESP32 peripherals
- ESP-IDF driver APIs

The goal is to progressively move from basic GPIO programming toward more advanced embedded concepts such as hardware timers, interrupts, peripheral drivers and real-time systems.

## Author

**Sannidhya Gupta**