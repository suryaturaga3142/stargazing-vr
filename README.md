# stargazing

Gang add some notes here when needed.


## Surya notes:

### SD Card

Part soldered. Has SPI (if needed) and SDIO interface. Use SDIO through PIO. Coded and in this branch, need to test when possible.

> [!IMPORTANT]
> Issue with the PIO environment. Need to fix compiling. 
> Also need to check the libraries and setup LFS.

Store the libraries from external sources such as FatFS, minmea in include/imported, src/imported. This way, we can differentiate between stuff we need to work on vs only ensure correctness.

### LCDs

8080-16b needs 24 pin FPC breakout. Both are there. They don't fit on the breadboard so we need female wires attached to soldered pin headers. It's 2x12 blocks. 

> [!WARNING]
> Do not connect the backlight pin Vcc to both LCDs. This pin uses a boost converter inside the driver to raise the voltage to the required amount. So, it draws way more current. We'll destroy the MCU if we try driving both BLs with a single output.

The only pins that need special handling are CS, Vcc, BL. CS needs to be different for both screens. Both can be pulled low to select both at all times. BL is a PWM for general brightness. We can connect both to the same PWM output. Vcc is complex. It's noise susceptible, so connect it to an external 3V3 source from a seperate regulator. Need it seperate or filtered with a Ferrite Bead and a decoupling capacitor.

Everthing else remains the same. The LCD can be tested with a single display if needed.

### IMU

BNO085 IMU has I2C and SPI. Given that SPI is full-duplex, it's faster. Also we need the game rotation vector, which is without the magnetometer bc magnetometer is susceptible to noise from other electronics.

> [!NOTE]
> It's now susceptible to drift so adding the tare button will help in resetting. The rotation vector is the absolute position quaternion relative to startup orientation. It can be given at upto $$400Hz$$ and uses the ```INT``` pin of the chip to generate an interrupt.

### PCB Design

RP2350 needs no change except moving of pushbuttons, LEDs, and addition of new buttons and an RGB LED. Keep the existing one, add on to it to expand the functionality without risking bad designs. Remove the GPIO pin headers and conserve on space.

> [!IMPORTANT]
> Use KiCad and store the hardware files in this repo. Add them to gitattributes to prevent changes to binary files. Use git LFS to prevent merge conflicts on them. The PCB aspect needs to have NO conflicts whatsoever. Keep it locked.

GPS module and LCD are both susceptible to EMI and stuff on PCBs, so they need to be away. The displays also need to be really modular. So keep a JST breakout on the PCB for the GPS. Keep FPC connectors on the PCB for the LCDs. 

> [!IMPORTANT]
> Remember how to handle the power! Filtering and seperate bc 1A draw is crazy.

The IMU needs to be hooked up on the PCB so keep it close to the required pins. Based on this and the final PCB orientation adjust the use of the rotation vector. For this, copy the Adafruit BNO085 IMU PCB, can be found online. But make it the exact same. Can ignore I2C peripheral.

The SD Card needs especially low latency, so copy the Adafruit SD Card circuit for SDIO and keep in near the chip.

> [!NOTE]
> Keep 1 USB-C for programming through the RP Debugger. The other USB (for RP OTG) can be removed. An external 5V supply needs to power everything. Need filtering and stuff.

Only new circuits needed would be related to the power supply. Polarity and overcurrent protection near the barrel jack, voltage regulators, and filtering for the LCD. Keep decoupling capacitors of equal size to prevent ringing. All the best!