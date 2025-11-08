#ifndef BNO08X_C_H
#define BNO08X_C_H

#include <stdint.h>
#include <stdbool.h>
#include "src/imported/sh2"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    sh2_Hal_t hal;
    void *hal_cookie;
    sh2_ProductIds_t prodIds;
    sh2_SensorValue_t sensorValue;
    uint8_t spi_cs_pin;
    void *spi_inst; // opaque pointer to spi instance (spi_inst_t*)
    bool initialized;
} BNO08x_c_t;

// Initialize for SPI. spi_inst should be a pointer to the spi instance (e.g., spi0)
// cs_pin is the chip-select GPIO number.
bool bno08x_begin_spi(BNO08x_c_t *dev, void *spi_inst, uint8_t cs_pin);

// Call regularly to service the sensor (routes to sh2_service())
void bno08x_service(BNO08x_c_t *dev);

// Enable a sensor report (wrapper around sh2_setSensorConfig)
bool bno08x_enableReport(BNO08x_c_t *dev, sh2_SensorId_t sensor, uint32_t interval_us, uint32_t sensorSpecific);

// Get last sensor value (returns true if available)
bool bno08x_getSensorValue(BNO08x_c_t *dev, sh2_SensorValue_t *outValue);

// Reset hardware pin (user must wire reset line and implement if needed)
void bno08x_hardwareReset(BNO08x_c_t *dev);

#ifdef __cplusplus
}
#endif

#endif // BNO08X_C_H
