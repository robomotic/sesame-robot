# The Sesame Robot Project 
<img width="95%" height="1728" alt="Untitled (7)" src="https://github.com/user-attachments/assets/9d7d88cb-b012-4611-9419-e154bb006078" />

**Greetings, from your new best friend.**

Sesame is an accessible Open-Source robotics project based on ESP32 and PCA9685 servo driver. This project is designed for makers and engineers of all skill levels. From absolute beginners to seasoned professionals, Sesame offers a dynamic platform designed to start working with walking robots.

This repository contains the CAD files for the body and the base firmware for the ESP32-based controller.

> [!WARNING]
> This Repository is under Construction! Details are subject to change.

## Features

*   **4-Legged Design:** Uses 8 servo motors (2 per leg) to achieve roughly 8 total degrees of freedom.
*   **Emotive Display:** Features a 128x64 OLED screen acting as a reactive face that syncs with movement.
*   **Fully Printable:** Designed entirely for 3D printing in PLA with minimal supports.
*   **Serial CLI:** Control the robot and trigger animations via a Serial Command Line Interface or the WIP web UI.
*   **Pre-programmed Emotes:** Includes animations for Walking, Waving, Dancing, Swimming, Pointing, Resting, and Standing.

## Hardware Requirements

To build a Sesame, you will need the following core components:

*   **Microcontroller:** ESP32 Development Board with WiFi connectivity.
*   **Actuators:** 8x MG90 Micro Metal Gear Servo Motors.
*   **Driver:** PCA9685 16-Channel PWM/Servo Driver.
*   **Display:** 0.96" I2C OLED Display (SSD1306 driver, 128x64).
*   **Power:** 5V Power Supply suitable for 8 servos (or 3x AAA battery pack).
*   **Body:** 3D Printed Parts (Files available in the `/CAD` directory).

## Fabrication

Sesame is designed to be printed in **PLA**. Most parts are designed to be printed without supports, though specific orientation guidelines are provided in the Build Guide Wiki.

*   **Material:** PLA / PLA+
*   **Infill:** 15-20%
*   **Supports:** Minimal (Check individual file notes)

## Firmware

The firmware provided in this repository controls the servo kinematics and the OLED face updates.

> [!NOTE]
> **Note from the Creator:** I am a hardware and design specialist, not a software wizard. This firmware works and provides a great platform, but it is basic. The community is highly encouraged to contribute, refactor, and improve the code logic.

### Dependencies

You will need the following libraries installed in your Arduino IDE:
*   `Wire.h` (Standard)
*   `Adafruit_PWMServoDriver.h`
*   `Adafruit_GFX.h`
*   `Adafruit_SSD1306.h`

### Setup

1.  **Board Setup:** Select your specific ESP32 board in the Arduino IDE Tools menu.
2.  **Connections:**
    *   Connect servos to the PCA9685 board (Channels 0-7).
    *   Connect the PCA9685 SDA/SCL pins to the standard I2C pins on your ESP32.
    *   Connect the OLED via I2C.
3.  **Network Configuration:** Open the firmware file and configure your desired Access Point name (SSID) and password, or set it to connect to your home router.
4.  **Upload:** Upload the firmware to the ESP32.
5.  **Monitor:** Open the Serial Monitor at **115200 baud** to see the assigned IP address.

### WiFi Controller Usage

Once the robot is powered on:

1.  Connect your phone or computer to the Robot's WiFi network (or the network the robot is connected to).
2.  Enter the IP address printed in the Serial Monitor into your web browser (e.g., `192.168.4.1`).
3.  A graphical interface will load featuring buttons for all available poses:
    *   **Rest:** Robot goes to sleep/rest mode.
    *   **Stand:** Robot stands up.
    *   **Walk:** Initiates the walking cycle.
    *   **Wave:** Robot waves hello.
    *   **Dance:** Robot performs a dance.
    *   **Swim:** Robot performs a swimming motion.
    *   **Point:** Robot points with a limb.

## Customizing Faces

The faces are stored as byte arrays in `PROGMEM` within the firmware file. You can create your own faces using any standard 128x64 bitmap converter (or MS Paint exported to C-array).
_I know this is dumb, currently working on a better way of doing this_

1.  Create your image (128x64 pixels, monochrome).
2.  Convert image to a hex array.
3.  Paste the array into the top of the firmware file.
4.  Call `updateFaceBitmap(your_new_bitmap_name)` in the code.

## Kits & Pre-Orders

If you do not have a 3D printer or prefer to get all the hardware in one box, limited pre-orders for full Sesame Build Kits are available. These kits include all the proper servos, drivers, fasteners, and printed parts to build your own Sesame.

*   **Shipping:** Q1 2026
*   **Store Link:** [TBA]

## Contributing

This robot is a platform for building new features and ideas. Since the current firmware is a basic implementation, pull requests are very welcome for:
*   Kinematics improvements
*   New animations
*   Improved Web UI/UX
*   Sensor integration (Ultrasonic, Gyro, etc.)

---

*Created by [Dorian Todd]. See my work and contact at https://www.doriantodd.com/
