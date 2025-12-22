# Stargazer VR

![Project Banner / Hero Image](docs/images/final_pic_1.jpg)
*A high-performance, standalone VR headset powered by the RP2350 that renders real-time star constellations based on precise geolocation and orientation.*

---

## 📖 Introduction

Light pollution is a problem not many people talk about. We wanted to shed light on this issue (pun intended), so we decided to build a VR headset that let the user see the night sky at the comfort of their home. 

This project is a wearable headset with an RP2350 microcontroller and two 3.5" TFT LCD displays to display stars and constellations in the sky, in the direction of the viewer sight. It uses a BNO085 IMU to track the user movements, a NEO-M10 GPS module to obtain present location, date, and time, and an SD Card to store a large catalog of star location data in a binary file.

### ⭐ Spotlight

![Spotlight 1](docs/images/final_pic_2.jpg)
*VR Headset assembled with a case, breadboards, and standard parts.*

![Spotlight 2](docs/images/stars_screen.jpg)
*View from a single screen during operation.*

---

## 📑 Table of Contents

Our Story

- [Features](#-features)
- [Instructions for Usage](#-instructions-for-usage)
- [Design Stages](#-design-stages)
- [Software Developement](#-software-development)
  - [The Rendering Pipeline](#-the-rendering-pipeline)
  - [Side Quest 1: Scripting](#-side-quest-1-scripting)
  - [Optimizations](#-optimizations)
- [Hardware Building](#-hardware-building)
  - [The Physical Design](#-the-physical-design)
  - [So many issues!](#-so-many-issues)
  - [Side Quest 2: PCB Design](#-side-quest-2-pcb-design)
  - [Putting it all together](#-putting-it-all-together)
- [Closing Thoughts](#-closing-thoughts)

<a id="-features"></a>
## 🚀 Features

The headset runs on a standard 9V battery, has two 3.5" TFT LCDs that display stars in front of the user with a 100 degree FOV. A single RGB LED is used to indicate the state of the VR Headset: booting, running, paused, critical error, etc. There are 3 buttons used to control the headset program flow. The Tare button recalibrates the IMU to face true North, the GPS toggle button switches between using real time GPS data and an in-built location and time (Purdue J2000), and the Pause/Play button does exactly that.

A single microSD card containing the required binary file format of star data can be inserted. Upon startup, this data is loaded into the RAM and processed. The catalog is limited to 15,000 stars. To produce the bin file, see [process_fits.py](data/scripts/process_fits.py), a script written to convert an fit file into the needed bin file.

The LCD Displays (ILI9486) as well as the microSD Card are controlled using SPI. The IMU (BNO085) is communicated with using I2C, more specifically SHTP. Finally, the GPS Module (Neo-M10) uses UART to transmit NMEA sentences.

---

<a id="-instructions-for-usage"></a>
## 📃 Instructions for Usage

If wiring up on a breadboard, refer to the KiCad schematic for exact resistor values to use for the RGB LED, and connections to configure the IMU in I2C mode. Everything else works as it should assuming the use of breakout boards. Any standard LDO can be used to provide the 5V supply from the 9V battery. 

> [!WARNING]
> Do not connect a 3V3 output of your MCU to the LCDs! SST drivers have boost converters in them (as you should if you're implementing your own driver) and the current draw from 2 LCDs is too much current draw. You need a seperate regulator to provide external 3V3 from your 9V external.

If you're using the PCB, it's a plug and play. It needs the Adafruit BNO085 IMU breakout board (they were out of stock when this was designed). The PCB also makes use of 16 bit 8080 for the displays, making it significantly faster. Instead of a 9V battery, it will need a LiPo battery rated for above 1A. It can be charged with a wall charger, and programmed using a Pico probe.

---

<a id="-design-stages"></a>
## 📐 Design Stages

Give a short flow of what all we went about doing. Basically a short story.

---

<a id="-software-development"></a>
## 🖥️ Software Development

We started with the software. Before wiring anything up, we began testing our design flow of using interrupts for timers, PWM, and GPIO. Then we went on to the more complex aspects like the communication with peripherals.

<a id="-the-rendering-pipeline"></a>
### 🚄 The Rendering Pipeline

Short desc. Mermaid flowchart?

<a id="-side-quest-1-scripting"></a>
### 🎬 Side Quest 1: Scripting

Short desc again. Show some images.

<a id="-optimizations"></a>
### 🪡 Optimizations

Give an idea of quaternion math, spatial culling, double buffering.

---

<a id="-hardware-building"></a>
## 🛠️ Hardware Building

While the code was being developed, stuff was getting wired up on the breadboard! Starting with the RGB LED, then moving to the IMU, GPS, SD Card, and finally, the displays. This was just the process of integration, of course. Everything was being developed and tested slowly at the same time.

<a id="-the-physical-design"></a>
### The Physical Design

Outline the choices made and what we started with

Put a pic.

<a id="-so-many-issues"></a>
### So many issues!

Short summary of problems we faced and how we adapted

<a id="-side-quest-2-pcb-design"></a>
### Side Quest 2: PCB Design

Short summary of making the PCB and what happened with it. Put a picture!

<a id="-putting-it-all-together"></a>
### Putting it all together

Finally, assembling the headset! Keep this short with pics.

---

<a id="-closing-thoughts"></a>
## 📦 Closing Thoughts

Showcase at spark and final thoughts on how the project could be improved and what not. End with group photo.
