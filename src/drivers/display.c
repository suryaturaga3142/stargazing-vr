/*******************************************************************************
 * @file        display.c
 * @brief       Implements the functionality for the LCD Display module.
 * @details     Complete set of functions needed to interact with the ILI9486 
 *              through the PIO.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be driven by interrupts and is not
 *              intended to be called from a blocking main loop.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "display.h"
#include "stdint.h"
#include "hardware/dma.h"
#include "hardware/irq.h" 

// ...

/* ---------------------------- Private Constants --------------------------- */
// ...
#define PIXEL_AMOUNT 1000
#define AMOUNT_OF_STARS 1000
#define PACKET_PER_STAR 20
#define ERASE_DMA_CAP (PIXEL_AMOUNT * PACKET_PER_STAR)
#define DRAW_DMA_CAP (PIXEL_AMOUNT * PACKET_PER_STAR)
#define FINAL_DMA_CAP (ERASE_DMA_CAP + DRAW_DMA_CAP)

#define COLOR_WHITE 0xFFFFu
#define COLOR_LIGHTGRAY 0xC618u
#define COLOR_BLACK 0x0000u

#define CMD_CASET 0x2A
#define CMD_PASET 0x2B
#define CMD_RAMWR 0x2C

/* ----------------------------- Private Variables -------------------------- */
// ...
static uint32_t erase_dma[ERASE_DMA_CAP]; //this array stores commands to erase the star data
static uint32_t draw_dma[DRAW_DMA_CAP]; //this array stores command to draw new stars
static uint32_t dma_transfer_list[FINAL_DMA_CAP]; //combines the erase array and draw array into one large array for dma

static int erase_index = 0; //how many stars got erased
static int draw_index = 0; //how many stars are being drawn
static int dma_count = 0; //how many packets in dma

static int old_star_data[AMOUNT_OF_STARS][2]; //stores old stars to compare to

//These variables store DMA and PIO info so you know which hardware to talk to
static uint dma_chan; 
static PIO dma_pio;
static uint dma_sm;
//volatile means it can be changed by interrupt and tells you if dma is busy or can take new
// data
static volatile bool dma_complete = true;

/* ----------------------------- Private Functions -------------------------- */
// ...
/**
 * @brief Talks to the PIO and takes data and formats it for the PIO. This is 
 * ordered in MSB so PIO will grab 31:15 and throw away 14:0.
 * 
 * @param payload this is the 16 bits of data like what color to send and draw
 * @param dc this is the command bit tells PIO whether its sending command or data
 * @return full 32 bit data for the PIO
 */
static inline uint32_t build_packet(uint16_t payload, uint8_t dc)
{
    return (((uint32_t)(dc & 1u)) << 31) | (((uint32_t)payload) << 15);
}

/**
 * @brief Take 32-packet from build packet and add it to erase dma
 * 
 * @param packet the 32 bit data that needs to be send to erase_list
 * @param dc this is the command bit tells PIO whether its sending command or data
 * @return NA
 */
static inline void append_erase_packet(uint32_t packet)
{
    if(erase_index < ERASE_DMA_CAP)
    {
        erase_dma[erase_index++] = packet;
    }
}
/**
 * @brief Takes 32 bit packet from build packet and adds it to draw dma
 * 
 * @param packet the 32 bit data that needs to be sent to draw list
 * @return NA
 */
static inline void append_draw_packet(uint32_t packet)

{
    if(draw_index < DRAW_DMA_CAP)
    {
        draw_dma[draw_index++] = packet;
    }
}

/**
 * @brief sends command packet to the erase list, for convience and readability
 * 
 * @param cmd This is the command like set address (dc = 0)
 * @return NA
 */
static inline void append_cmd_to_erase(uint16_t cmd)
{
    append_erase_packet(build_packet(cmd, 0));
}

/**
 * @brief sends data to the erase list, for convience and readability
 * 
 * @param param the data packet being sent (dc = 1)
 * @return NA
 */
static inline void append_param_to_erase(uint16_t param)
{
    append_erase_packet(build_packet(param, 1));
}

/**
 * @brief sends command packet to the draw list, for convience and readability
 * 
 * @param cmd This is the command like set address (dc = 0)
 * @return NA
 */
static inline void append_cmd_to_draw(uint16_t cmd)
{
    append_draw_packet(build_packet(cmd, 0));
}

/**
 * @brief sends data to the draw list, for convience and readability
 * 
 * @param param the data packet being sent (dc = 1)
 * @return NA
 */
static inline void append_param_to_draw(uint16_t param)
{
    append_draw_packet(build_packet(param, 1));
}

/**
 * @brief Only runs when DMA finishes sending all packets from dma_transfer_list
 * 
 * @param NA
 * @return NA
 */
static void dma_complete_isr()
{
    // Clear the interrupt request flag
    dma_irqn_acknowledge_channel(DMA_IRQ_0, dma_chan);
    
    // Signal that the transfer is complete
    dma_complete = true;
}

/**
 * @brief Erases the stars on the screen, draws a black pixel over that area
 * 
 * @param x0 start column (left edge) of the 3x3 box
 * @param x1 end column (right edge) of the 3x3 box
 * @param y0 start row (top edge) of 3x3 box
 * @param y1 end row (bottom edge) of 3x3 box
 * @param pixels pointer ot array of pixel data
 * @param npixels total number of pixels which should be 9
 * @return Description of what this function returns.
 */
static void append_window_and_pixels_erase(int x0, int x1, int y0, int y1, const uint16_t *pixels, int npixels)
{
    append_cmd_to_erase(CMD_CASET);
    append_param_to_erase((x0 >> 8) & 0xFF);
    append_param_to_erase(x0 & 0xFF);
    append_param_to_erase((x1 >> 8) & 0xFF);
    append_param_to_erase(x1 & 0xFF);

    append_cmd_to_erase(CMD_PASET);
    append_param_to_erase((y0 >> 8) & 0xFF);
    append_param_to_erase(y0 & 0xFF);
    append_param_to_erase((y1 >> 8) & 0xFF);
    append_param_to_erase(y1 & 0xFF);

    append_cmd_to_erase(CMD_RAMWR);

    for (int i = 0; i < npixels; ++i)
    {
        append_param_to_erase(pixels[i]);
    }
}

static void append_window_and_pixels_draw(int x0, int x1, int y0, int y1, const uint16_t *pixels, int npixels)
{
    append_cmd_to_draw(CMD_CASET);
    // Send 4 parameters for CASET
    append_param_to_draw((uint16_t)(x0 >> 8));   // Start Column MSB
    append_param_to_draw((uint16_t)(x0 & 0xFF)); // Start Column LSB
    append_param_to_draw((uint16_t)(x1 >> 8));   // End Column MSB
    append_param_to_draw((uint16_t)(x1 & 0xFF)); // End Column LSB

    append_cmd_to_draw(CMD_PASET);
    // Send 4 parameters for PASET
    append_param_to_draw((uint16_t)(y0 >> 8));   // Start Page MSB
    append_param_to_draw((uint16_t)(y0 & 0xFF)); // Start Page LSB
    append_param_to_draw((uint16_t)(y1 >> 8));   // End Page MSB
    append_param_to_draw((uint16_t)(y1 & 0xFF)); // End Page LSB

    append_cmd_to_draw(CMD_RAMWR);
    for (int i = 0; i < npixels; ++i)
    {
        append_param_to_draw(pixels[i]);
    }
}

/**
 * @brief Draws the star and puts it in the array
 * 
 * @param out_pixel the 9 pixels to draw to
 * @return NA
 */
static void make_star_pixels(uint16_t out_pixels[9]) 
{
    out_pixels[0] = COLOR_LIGHTGRAY;
    out_pixels[1] = COLOR_WHITE;
    out_pixels[2] = COLOR_LIGHTGRAY;
    out_pixels[3] = COLOR_WHITE;
    out_pixels[4] = COLOR_WHITE;
    out_pixels[5] = COLOR_WHITE;
    out_pixels[6] = COLOR_LIGHTGRAY;
    out_pixels[7] = COLOR_WHITE;
    out_pixels[8] = COLOR_LIGHTGRAY;
}

/**
 * @brief Draws black to the star by sending it to array
 * 
 * @param out_pixels the 9 pixels to draw to
 * @return NA
 */
static void make_erase_pixels(uint16_t out_pixels[9])
{
    for (int i = 0; i < 9; i++)
    {
        out_pixels[i] = COLOR_BLACK;
    }
}
 
/**
 * @brief checks for boundaries to make sure you never write outside of bounds of lcd
 * 
 * @param x what x coordinate is being written
 * @return return back good x value
 */
static inline int clamp_x(int x) 
{
    if (x < 0) 
    {
        return 0; 
    }
    if (x > 319) 
    {
        return 319;
    }
    return x; 
}

/**
 * @brief checks for boundaries to make sure you never write outside of bounds of lcd
 * 
 * @param y what y coordinate is being written
 * @return return back good y value
 */
static inline int clamp_y(int y) 
{
    if (y < 0) 
    {
        return 0; 
    }
    if (y > 479) 
    {
        return 479; 
    }
    return y; 
    
}

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief erase the star from the coordinates set based on the center of the star
 * 
 * @param x coordinate coming in from the matrix
 * @param y cooordinate coming in from the matrix
 * @return NA
 */
void erase_the_star(int x, int y)
{
    // compute 3x3 bounds (clamped)
    int x0 = clamp_x(x - 1);
    int x1 = clamp_x(x + 1);
    int y0 = clamp_y(y - 1);
    int y1 = clamp_y(y + 1);

    // create 3x3 background pixel block
    uint16_t pixels[9];
    make_erase_pixels(pixels);

    // append to erase_dma list
    append_window_and_pixels_erase(x0, x1, y0, y1, pixels, 9);
}

/**
 * @brief draw the star from the coordinates set based on the center of the star
 * 
 * @param x coordinate coming in from the matrix
 * @param y cooordinate coming in from the matrix
 * @return NA
 */
void draw_the_star(int x, int y)
{
    int x0 = clamp_x(x - 1);
    int x1 = clamp_x(x + 1);
    int y0 = clamp_y(y - 1);
    int y1 = clamp_y(y + 1);

    uint16_t pixels[9];
    make_star_pixels(pixels);

    append_window_and_pixels_draw(x0, x1, y0, y1, pixels, 9);
}

/**
 * @brief The logic behind everything, compares the star values, then 
 * checks if it needs to erase the star and if not then it draws the star 
 * and sets the old star values
 * 
 * @param matrix full of all the star coordinates.
 * @return NA
 */
void place_new_stars(int matrix[AMOUNT_OF_STARS][2])
{
    // Reset buffers
    erase_index = 0;
    draw_index  = 0;
    dma_count = 0;

    for (int i = 0; i < AMOUNT_OF_STARS; ++i)
    {
        int old_x = old_star_data[i][0];
        int old_y = old_star_data[i][1];
        int new_x = matrix[i][0];
        int new_y = matrix[i][1];

        // If changed (note: -1,-1 indicates "no previous star" — skip erase)
        if (old_x != new_x || old_y != new_y)
        {
            if (old_x >= 0 && old_y >= 0)
            {
                // queue an erase sequence for the old center
                erase_the_star(old_x, old_y);
            }
            // queue a draw sequence for the new center
            draw_the_star(new_x, new_y);

            // update old position
            old_star_data[i][0] = new_x;
            old_star_data[i][1] = new_y;
        }
    }

    // Now concatenate erase_dma then draw_dma into dma_transfer_list
    // Make sure we don't exceed final capacity
    int e = erase_index;
    int d = draw_index;
    if ((e + d) > FINAL_DMA_CAP) {
        // clamp (shouldn't happen if capacities sized correctly)
        if (e > FINAL_DMA_CAP)
        {
            e = FINAL_DMA_CAP;
        }
        if (d > (FINAL_DMA_CAP - e))
        {
            d = FINAL_DMA_CAP - e;
        }
    }

    // copy
    for (int i = 0; i < e; ++i)
    {
        dma_transfer_list[i] = erase_dma[i];
    }
    for (int i = 0; i < d; ++i) 
    {
        dma_transfer_list[e + i] = draw_dma[i];
    }

    dma_count = e + d;

    // dma_transfer_list[0..dma_count-1] is now the full ordered packet sequence for DMA.
    // Caller must start DMA to send dma_transfer_list with count = dma_count.
}

// Utility: getter for DMA pointer/count so caller can start DMA
uint32_t *display_get_dma_ptr(void)
{
    return dma_transfer_list; 
}
int display_get_dma_count(void)
{
    return dma_count;
}

// Initialization helper: clear old_star_data to invalid markers
void display_init_star_cache(void)
{
    for (int i = 0; i < AMOUNT_OF_STARS; ++i) {
        old_star_data[i][0] = -1;
        old_star_data[i][1] = -1;
    }
}

void display_dma_init(PIO pio, uint sm) 
{
    dma_pio = pio;
    dma_sm = sm;
    
    // 1. Claim a DMA channel
    dma_chan = dma_claim_unused_channel(true); // `true` = required

    // 2. Get the default DMA configuration
    dma_channel_config c = dma_channel_get_default_config(dma_chan);

    // 3. Configure the DMA
    //    - Transfer 32-bit words (matches our PIO and build_packet)
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    //    - Read from memory, incrementing the read address
    channel_config_set_read_increment(&c, true);
    //    - Write to a fixed peripheral address (the PIO TX FIFO)
    channel_config_set_write_increment(&c, false);
    
    //    - Pace the DMA transfers using the PIO's TX DREQ
    //      (This is the magic that makes it wait for the PIO FIFO)
    uint dreq = (pio == pio0) ? DREQ_PIO0_TX0 : DREQ_PIO1_TX0;
    channel_config_set_dreq(&c, dreq + sm);

    // 4. Apply this config to the channel
    //    We set the write address *now* (it's permanent)
    //    The read address and count are set just before transfer
    dma_channel_configure(
        dma_chan,
        &c,
        &pio->txf[sm], // Permanent Write address: PIO TX FIFO
        NULL,           // Read address: Will be set later
        0,              // Transfer count: Will be set later
        false           // Don't trigger yet
    );

    // 5. Set up the DMA interrupt
    //    - Tell the DMA to fire an interrupt on IRQ 0 when it's done
    dma_irqn_set_channel_enabled(DMA_IRQ_0, dma_chan, true);
    //    - Set our dma_complete_isr as the function to call
    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_isr);
    //    - Enable the interrupt in the processor
    irq_set_enabled(DMA_IRQ_0, true);
}

bool display_dma_is_busy() 
{
    return !dma_complete;
}

void display_dma_start_transfer() 
{
    // Don't start a new transfer if the old one is still running
    if (!dma_complete) {
        return;
    }
    
    // We are now busy
    dma_complete = false;
    
    // 1. Set the source address
    dma_channel_set_read_addr(
        dma_chan,
        display_get_dma_ptr(),  // Get the pointer to dma_transfer_list
        false                   // Don't trigger yet
    );

    // 2. Set the transfer count and TRIGGER the DMA to start!
    //    The DMA will now wait for the PIO DREQ and start feeding
    //    words from the list into the PIO FIFO.
    dma_channel_set_trans_count(
        dma_chan,
        display_get_dma_count(), // Get the number of packets
        true                     // TRIGGER!
    );
}
//cfunc - this is for func header
//csrc - is for file header
//chdr - is for header file