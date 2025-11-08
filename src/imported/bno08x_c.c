#include "bno08x_c.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sh2/include/sh2.h"
#include "sh2/include/sh2_SensorValue.h"
#include "sh2/include/sh2_err.h"
#include "sh2/include/sh2_util.h"
#include "sh2/include/shtp.h"
#include "sh2/include/sh2_hal.h"

// NOTE: This is a best-effort C port that provides a lightweight SPI HAL for the CEVA sh2 layer.
// It may need small adjustments for your platform and wiring.

// Forward declarations for HAL callbacks
static int spi_hal_open(sh2_Hal_t *self);
static void spi_hal_close(sh2_Hal_t *self);
static int spi_hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us);
static int spi_hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len);
static uint32_t hal_getTimeUs(sh2_Hal_t *self);

// Simplified get time in microseconds
static uint32_t hal_getTimeUs(sh2_Hal_t *self)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000000u + ts.tv_nsec/1000u);
}

bool bno08x_begin_spi(BNO08x_c_t *dev, void *spi_inst, uint8_t cs_pin)
{
    if (!dev) return false;
    memset(dev,0,sizeof(*dev));
    dev->spi_inst = spi_inst;
    dev->spi_cs_pin = cs_pin;

    dev->hal.open = spi_hal_open;
    dev->hal.close = spi_hal_close;
    dev->hal.read = spi_hal_read;
    dev->hal.write = spi_hal_write;
    dev->hal.getTimeUs = hal_getTimeUs;
    dev->hal.cookie = dev;
    
    // Open sh2 device
    int rc = sh2_open(&dev->hal);
    if (rc != 0) {
        // failed
        return false;
    }

    // turn device on
    sh2_devOn(&dev->hal);

    dev->initialized = true;
    return true;
}

void bno08x_service(BNO08x_c_t *dev)
{
    if (!dev || !dev->initialized) return;
    sh2_service(&dev->hal);
}

bool bno08x_enableReport(BNO08x_c_t *dev, sh2_SensorId_t sensor, uint32_t interval_us, uint32_t sensorSpecific)
{
    if (!dev || !dev->initialized) return false;
    sh2_SensorConfig_t cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.reportId = sensor;
    cfg.sampleInterval_us = interval_us;
    cfg.sensitivity = 0;
    cfg.sensorSpecific = sensorSpecific;
    int rc = sh2_setSensorConfig(&dev->hal, &cfg);
    return (rc == 0);
}

bool bno08x_getSensorValue(BNO08x_c_t *dev, sh2_SensorValue_t *outValue)
{
    if (!dev || !dev->initialized || !outValue) return false;
    // copy latest sensorValue
    *outValue = dev->sensorValue;
    return true;
}

void bno08x_hardwareReset(BNO08x_c_t *dev)
{
    // Hardware reset behavior is platform specific. Leave as a placeholder.
    (void)dev;
}

// --- Very small SPI HAL implementation using Linux-like spidev or platform SPI.
// The implementation below uses POSIX-like read/write of /dev/spidev if spi_inst is a file descriptor (int*)
// If you have the Raspberry Pi Pico SDK or another platform, replace these implementations
// with platform specific spi_write_read_blocking/gpio_put calls.

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

static int spi_fd_from_cookie(sh2_Hal_t *self)
{
    BNO08x_c_t *dev = (BNO08x_c_t*)self->cookie;
    if (!dev || !dev->spi_inst) return -1;
    // allow spi_inst to be a pointer to an int file-descriptor
    int fd = *(int*)dev->spi_inst;
    return fd;
}

static int spi_hal_open(sh2_Hal_t *self)
{
    // If the user provided a file descriptor in spi_inst we assume it's already open/configured.
    (void)self;
    return 0;
}

static void spi_hal_close(sh2_Hal_t *self)
{
    (void)self;
}

static int spi_hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
    int fd = spi_fd_from_cookie(self);
    if (fd < 0) return -1;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)pBuffer,
        .rx_buf = 0,
        .len = len,
        .cs_change = 0,
    };
    int res = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    return (res < 0) ? -1 : (int)len;
}

static int spi_hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us)
{
    int fd = spi_fd_from_cookie(self);
    if (fd < 0) return -1;
    // perform full-duplex transfer sending zeros to receive data
    uint8_t tx[len];
    memset(tx, 0, len);
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)pBuffer,
        .len = len,
        .cs_change = 0,
    };
    int res = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (t_us) *t_us = hal_getTimeUs(self);
    return (res < 0) ? -1 : (int)len;
}
