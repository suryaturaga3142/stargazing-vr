/*******************************************************************************
 * @file        imu.c
 * @brief       Implements the functionality for the IMU module.
 * @details     Complete set of functions needed to interact with the BNO080.
 * 
 * @author      LED Chasers
 * @date        2025-11-02
 * 
 * @note        This module is designed to be driven by interrupts and is not
 *              intended to be called from a blocking main loop. Functions here
 *              need to be of highest priority to minimize latency.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "imu.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdio.h> //TEMP
#include "pico/stdlib.h" //TEMP
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...
spi_inst_t* SPI_BUS = spi1;
const int SPI_IMU_SCK = 14; // SCK pin number for the IMU (SCL)
const int SPI_IMU_CSn = 13; // CSn pin number for the IMU (CS)
const int SPI_IMU_RX = 12; // RX pin number for the IMU (SDA)
const int SPI_IMU_TX = 15; // TX pin number for the IMU (DI)
const int IMU_INTR = 17; // GPIO pin to receive the interrupts from the IMU
const int IMU_RST = 30; // GPIO pin tied to IMU reset
//const int SPI_BAUDRATE = 4000000; //4MHz
const int SPI_BAUDRATE = 100000; //1kHz

/* ----------------------------- Private Variables -------------------------- */
uint8_t sequence_num = 0;
bno085_packet_t last_packet;

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

/**
  @brief         Initialize pins of RP3250 for SPI communication
*/
void init_spi_for_imu()
{
    //No data ready to be read yet
    imu_data_ready_flag = 0;

    //Set initial rotation vector to identity vector
    current_rotation_vector.w = 1.0f;
    current_rotation_vector.x = 0.0f;
    current_rotation_vector.y = 0.0f;
    current_rotation_vector.z = 0.0f;

    //Initialize GPIO pins for SPI
    gpio_init_mask((1 << SPI_IMU_SCK) | (1 << SPI_IMU_CSn) | (1 << SPI_IMU_RX) | (1 << SPI_IMU_TX));
    gpio_set_function_masked((1 << SPI_IMU_SCK) | (1 << SPI_IMU_RX) | (1 << SPI_IMU_TX), GPIO_FUNC_SPI); //Don't include SPI_IMU_CSn
    gpio_set_dir(SPI_IMU_CSn, true); //Set CS pin as output
    gpio_put(SPI_IMU_CSn, 1); 

    //Enable interrupt when IMU_INTR pin is pulled low
    gpio_init(IMU_INTR);
    gpio_set_dir(IMU_INTR, false);
    gpio_add_raw_irq_handler(IMU_INTR, imu_isr);
    irq_set_enabled(IO_IRQ_BANK0, true);
    gpio_set_irq_enabled(IMU_INTR, GPIO_IRQ_EDGE_FALL, true);

    //Initialize reset pin
    gpio_init(IMU_RST);
    gpio_set_dir(IMU_RST, true);
    //Reset IMU
    gpio_put(IMU_RST, 0);
    sleep_ms(60);
    gpio_put(IMU_RST, 1);
    sleep_ms(100);

    //Initialize SPI
    spi_init(SPI_BUS, SPI_BAUDRATE);
    spi_set_format(SPI_BUS, 8, 1, 1, SPI_MSB_FIRST); // BNO085 uses SPI Mode 3 (CPOL=1, CPHA=1)

    //Configure IMU to generate interrupts at 100Hz
    //TODO: Turn packet into a struct?
    uint8_t packet[] = {
        // SHTP header (4 bytes)
        0x15, 0x00,      //Length = 21 bytes (LSB first)
        CHANNEL_CONTROL, //Channel num = 2
        sequence_num,    //Sequence num = 0

        // Payload (Set Feature Command)
        CMD_SET_FEATURE,        //Command ID = 0xFD
        REPORT_GAME_ROTATION_VECTOR, //Report ID = 0x08
        0x00, 0x00,                  //Feature flags
        0x10, 0x27, 0x00, 0x00, //Report interval = 10000 us (100 Hz)
        0x00, 0x00, 0x00, 0x00,        
        0x00, 0x00, 0x00, 0x00,
        0x00
    };
    gpio_put(SPI_IMU_CSn, 0); //Pull chip select low
    spi_write_blocking(SPI_BUS, packet, sizeof(packet));
    gpio_put(SPI_IMU_CSn, 1); //Pull chip select high
    sleep_ms(10); // Wait for processing

    // Verify features were enabled by checking for response of 0xF1 on channel 2
    bno085_packet_t response;
    bool got_response = false;
    for(int i = 0; i < 10 && !got_response; i++) { // Try up to 10 times
        if(bno085_service_fifo_ch2(&response)) {
            if(response.channel == CHANNEL_CONTROL && 
               response.payload[0] == CMD_GET_FEATURE_RESPONSE) {
                got_response = true;
                printf("Features enabled successfully\n");
            }
        }
        sleep_ms(1);
    }
    if(!got_response) {
        printf("Warning: Feature enable response not received\n");
    }
    
    return;
}

/**
  @brief        Handle interrupt and raise flag that there is data ready to be read
  @note         Should be triggered with interrupt
*/
void imu_isr()
{
    gpio_acknowledge_irq(IMU_INTR, GPIO_IRQ_EDGE_FALL);
    imu_data_ready_flag = 1;
    return;
}

/**
  @brief         Read one full packet of data from BNO085 with SPI
  @param
*/
int bno085_read_packet(bno085_packet_t *pkt) {
    uint8_t header[4];

    // Start SPI transaction
    gpio_put(SPI_IMU_CSn, 0);
    
    // Read SHTP header (4 bytes)
    // First two bytes are length (LSB first)
    // Third byte is Channel ID
    // Fourth byte is Sequence number
    spi_read_blocking(SPI_BUS, 0, header, 4);
    
    // Parse header
    uint16_t total_len = header[0] | (header[1] << 8);
    total_len &= 0x7FFF; // Mask out top bit (continuation bit)
    pkt->channel = header[2];
    pkt->sequence = header[3];
    pkt->payload_len = total_len > 4 ? total_len - 4 : 0;
    if (pkt->payload_len > 255) pkt->payload_len = 255;

    //Read payload
    spi_read_blocking(SPI_BUS, 0, pkt->payload, pkt->payload_len);

    // End SPI transaction
    gpio_put(SPI_IMU_CSn, 1);

    pkt->valid = (total_len >= 4);
    return pkt->valid;
}

/**
    @brief          Clears every pending SHTP packet in FIFO.
    @return         Returns true only when a channel-3 packet was found and stored in out_pkt.
 */
bool bno085_service_fifo(bno085_packet_t *out_pkt) {
    bool got_report = false;
    bno085_packet_t pkt;

    while (!gpio_get(IMU_INTR)) {           // while INT low → FIFO not empty
        if (!bno085_read_packet(&pkt)) break;  // failed read?
        if (pkt.channel == 3) {                // sensor-report channel
            *out_pkt = pkt;
            got_report = true;
        }
    }
    return got_report;
}

/**
    @brief          Clears every pending SHTP packet in FIFO.
    @return         Returns true only when a channel-2 packet was found and stored in out_pkt.
 */
bool bno085_service_fifo_ch2(bno085_packet_t *out_pkt) {
    bool got_report = false;
    bno085_packet_t pkt;

    while(gpio_get(IMU_INTR))
    {
        sleep_ms(1);
    }

    while (!gpio_get(IMU_INTR)) {           // while INT low → FIFO not empty
        if (!bno085_read_packet(&pkt)) break;  // failed read?
        if (pkt.channel == 2) {                // sensor-report channel
            *out_pkt = pkt;
            got_report = true;
        }
    }
    return got_report;
}

/**
  @brief        Parse IMU rotation vector data into quaternion
*/
void parse_imu_rotation_data()
{
    imu_data_ready_flag = 0;
    // Drain FIFO, keep only channel-3 packets
    if (bno085_service_fifo(&last_packet)) {
        uint8_t report_id = last_packet.payload[0];
        if (report_id == REPORT_GAME_ROTATION_VECTOR) {
            current_rotation_vector.w = (last_packet.payload[1] | (last_packet.payload[2] << 8)) / 16384.0f;
            current_rotation_vector.x = (last_packet.payload[3] | (last_packet.payload[4] << 8)) / 16384.0f;
            current_rotation_vector.y = (last_packet.payload[5] | (last_packet.payload[6] << 8)) / 16384.0f;
            current_rotation_vector.z = (last_packet.payload[7] | (last_packet.payload[8] << 8)) / 16384.0f;
            printf("\nw: %f,\tx: %f,\ty: %f,\tz: %f", current_rotation_vector.w, current_rotation_vector.x, current_rotation_vector.y, current_rotation_vector.z);
        }
    }
    return;
}


//THIS FUNCTION IS CURRENTLY NOT IN USE, GOING TO EVENTUALLY DELETE
/**
  @brief        Read data from the IMU
*/
void read_imu_data()
{
    //Reading data so set flag back to 0
    imu_data_ready_flag = 0;
    //printf("\nFlag value: %d", imu_data_ready_flag);

    // Pull CS low to start transaction
    gpio_put(SPI_IMU_CSn, 0);
    
    // Read header (4 bytes)
    uint8_t header[4];
    spi_read_blocking(SPI_BUS, 0, header, 4);
    
    // Parse header
    uint16_t packet_length = ((uint16_t)header[0] | ((uint16_t)header[1] << 8)) & 0x7FFF; //Exclude bit 15w which is the continuation bit
    uint8_t channel = header[2];
    uint8_t seq_num = header[3];
    //printf("\nChannel: %d, Seq_num: %d, packet_length: %d", channel, seq_num, packet_length);
    
    uint16_t payload_length = packet_length - 4;

    // Allocate buffer for payload
    uint8_t payload[payload_length];
    
    // Read payload
    if (payload_length > 0) {
        spi_read_blocking(SPI_BUS, 0, payload, payload_length);
    }
    
    // Pull CS high to end transaction
    gpio_put(SPI_IMU_CSn, 1);
    
    // Process the packet based on channel
    if ((channel == CHANNEL_REPORTS)) {// && (payload[0] == REPORT_GAME_ROTATION_VECTOR)) {
        printf("\nCorrect channel and ID");
        //Divide by 2^14 to normalize quaternion
        current_rotation_vector.w = (payload[1] | (payload[2] << 8)) / 16384.0f;
        current_rotation_vector.x = (payload[3] | (payload[4] << 8)) / 16384.0f;
        current_rotation_vector.y = (payload[5] | (payload[6] << 8)) / 16384.0f;
        current_rotation_vector.z = (payload[7] | (payload[8] << 8)) / 16384.0f;
        printf("\nw: %f,\tx: %f,\ty: %f,\tz: %f", current_rotation_vector.w, current_rotation_vector.x, current_rotation_vector.y, current_rotation_vector.z);
    }
    //else {
        //printf("\nChannel: %d\tReport: %d", channel, payload[0]);
    //}
    
    return;
}