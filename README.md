# Stargazer VR: Embedded Augmented Reality Star Tracker

![Project Banner / Hero Image](docs/images/project_hero.png)
*A high-performance, standalone VR headset powered by the RP2350 that renders real-time star constellations based on precise geolocation and orientation.*

---

## 📖 Introduction

[INSERT INTRO HERE]  
*(Write a short paragraph here introducing the project. Example: "Stargazer VR was born out of a desire to merge embedded systems with astronomy. Unlike mobile apps that rely on phone sensors, this project is a dedicated hardware solution running bare-metal firmware on a custom PCB to deliver low-latency star tracking without an operating system.")*

---

## 📑 Table of Contents
- [Features](#-features)
- [System Architecture](#-system-architecture)
- [Hardware & PCB Design](#-hardware--pcb-design)
- [Software Engineering](#-software-engineering)
  - [The Rendering Pipeline](#the-rendering-pipeline)
  - [Spatial Culling & Optimization](#spatial-culling--optimization)
  - [PIO Display Driver](#pio-display-driver)
- [Development Tools](#-development-tools)
- [Getting Started](#-getting-started)
- [Credits](#-credits)

---

## 🚀 Features

* **Standalone Operation:** No PC or Smartphone required; runs entirely on the RP2350 microcontroller.
* **Real-Time Sensor Fusion:** Combines BNO08x IMU data (Game Rotation Vectors) with NEO-M10 GPS data for drift-free orientation.
* **Astronomical Accuracy:** Calculates Sidereal Time and performs quaternion rotations to map J2000 star data to the user's local horizon.
* **High-Performance Rendering:** Custom 3D projection engine written in C/C++ capable of rendering thousands of stars.
* **Optimized Memory Management:** Uses spatial hashing to load and render only the stars currently visible in the Field of View (FOV).
* **Custom Hardware:** Designed on a custom PCB featuring a high-speed 16-bit 8080 parallel LCD interface.

---

## ⚙️ System Architecture

The system operates on a "super-loop" architecture heavily reliant on interrupts and DMA to maintain high frame rates.

### The Data Flow
1.  **Initialization:** The system loads a custom binary star catalog (`stars.bin`) from the SD card into a sorted RAM buffer.
2.  **Mechanics:**
    * **GPS:** Acquires Latitude, Longitude, and UTC Time.
    * **Time Conversion:** UTC is converted to Local Sidereal Time (LST) to determine Earth's rotation relative to the stars.
    * **IMU:** Acquires the user's head orientation (Quaternion).
3.  **Fusion:** The system calculates a final `Model_View_Projection` quaternion by combining Earth's tilt, the user's location, sidereal time, and head orientation.
4.  **Rendering:** 3D unit vectors of stars are rotated, projected onto a 2D plane, and pushed to the display via DMA.

![System Block Diagram](docs/images/system_architecture_diagram.png)
*[Insert a block diagram showing the data flow between GPS, IMU, CPU, and Display]*

---

## 🛠 Hardware & PCB Design

This project is built around a custom Printed Circuit Board (PCB) designed to minimize footprint while maximizing signal integrity for high-speed buses.

### Key Components
* **MCU:** Raspberry Pi RP2350 (Dual-core ARM Cortex-M33).
* **IMU:** BNO085 (High-precision sensor fusion hub) via I2C.
* **GPS:** U-Blox NEO-M10 (GNSS) via UART.
* **Storage:** MicroSD Card Slot via SPI.
* **Display:** IPS LCD Module using a 16-bit 8080 Parallel Interface.

### PCB Layout Highlights
The PCB was designed to handle the high-speed signaling required for the display.
* **Display Interface:** The 16-bit parallel bus requires length-matched traces to ensure data integrity at high switching frequencies.
* **Power Management:** Dedicated regulators for the sensitive GPS/IMU rails versus the noisy digital logic.

![PCB Top Layer Render](docs/images/pcb_render_top.png)
*[Insert image of your PCB 3D Render or Photograph]*

![PCB Schematic Snippet](docs/images/schematic_snippet.png)
*[Insert a snippet of an interesting part of your schematic, perhaps the MCU or Display connector]*

---

## 💻 Software Engineering

The firmware is written in C/C++ using the Pico SDK. It emphasizes bare-metal performance optimizations to handle 3D math and rendering on a microcontroller.

### The Rendering Pipeline
We utilize **Quaternions** exclusively for rotation math to avoid gimbal lock and reduce computational overhead compared to Euler angles.

$$v' = q_{final} \cdot v_{star} \cdot q_{final}^{-1}$$

The `rendering.c` module performs a "Perspective Divide" to map the 3D rotated vectors onto the 2D screen coordinates $(x, y)$.

### Spatial Culling & Optimization
To render 9,000+ stars efficiently on an embedded device, iterating through the entire array every frame is impossible.

* **Spatial Hashing:** The sky is divided into a grid of "Sky Patches" (24 RA divisions x 12 Dec divisions).
* **Pre-Sorting:** On startup, the `sd_card.c` module sorts stars into these bins in RAM.
* **Frustum Culling:** During the render loop, the camera's forward vector determines which bins are visible. The system **only** processes stars within those specific bins, reducing the workload by ~90%.

![Visual of Spatial Grid](docs/images/spatial_hashing_diagram.png)
*[Insert diagram illustrating the sphere divided into patches]*

### PIO Display Driver
The RP2350's Programmable I/O (PIO) blocks are used to drive the LCD.
* **Protocol:** 16-bit 8080 Parallel (Intel style).
* **DMA Transfer:** The CPU calculates the frame, writes to a buffer, and triggers a Direct Memory Access (DMA) channel.
* **Result:** The PIO state machine pushes pixel data to the screen independently, leaving the CPU 100% free to calculate the next frame's physics.

---

## 🧰 Development Tools

A suite of Python scripts was developed to process data and simulate the hardware environment during development.

### Required Packages
```bash
pip install numpy pandas pyside6 pyqtgraph serial astropy

To be continued.