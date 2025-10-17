# stargazing

Gang add some notes here when needed.

Understand PIO
.program lcd_parallel
.side_set 1 opt

.wrap_target
    pull block
    out pins, 18
    nop     side 0 [1];
    nop     side 1 [1];
.wrap_target

".program lcd_parallel" is how we start the program and gives a name to the program
".side_set 1 opt" sets aside a single pin, in this case the pin will be the WR, and allow this pin to be controlled while the main function is going simuationously. The "opt" means this is optional 
".wrap" is like a loop, the program will loop through this when it begins
"out pins, 18" sends 18 bits from the OSR to the data pins and auto pulls to ensure the OSR automatically refills
".nop side 0 [1]" drives the WR low for 2 cycles ([1] means 2 cycles[1 + 1 delay]), the 1 means drive WR high

void lcd_parallel_program_init(PIO pio, uint sm, uint offset, uint pin_data_base, uint pin_wr, float clkdiv)

PARAMETERS:
PIO pio - hardware has two pio blocks, pio0 and pio1, select one
uint sm - which state machine which is like a simple robot built
          for an extremely simple task like moving bits
uint offset - where the PIO program is in the PIO block's instruction
              memory
uint pin_data_base - this is the GPIO number being used on the RP2350
uint pin_wr - which pin will be used for WR strobe
float clkdiv - how much to slow down the PIO state machine, formula is
               (1/system_clock) * clkdiv
pio_sm_config c - it is a struct containing pin bases, shift behavior,
                  clock divider, auto pull setting, basically making a
                  sm_config object for PIO with prefilled defaults

FUNCTIONS:
sm_config_set_out_pins() - tells the SM which physical pins to use for 
                           the out instructions and how many consecutive pins
sm_config_set_sideset_pins() - maps the PIO program side set to a 
                               particular pin which here would be the WR pin
sm_config_set_out_shift() - shift_right = true means OSR will shift bits
                            right(sets the direction)
                            autopull = true enables automatic pulling from TX FIFO into the OSR when OSR has fewer bits than threshold
                            pull_threshold = 18, when the OSR has fewer than 18 bits left, the hardware is going to take in new data
sm_config_set_clkdiv() - sets the statemachine clock divider
pio_sm_set_consecutive_pindir() - sets the direction for consecutive 
                                  range of pins used by State Machine
pio_sm_set_pindirs_with_mask() - set direction of specific pins using a
                                 mask which here we need to set direction of pin_wr to an output pin, this is the better alternative to gpio_set_dir(i do not know why)
pio_sm_init() - writes the config register to the state machine and 
                prepares the state machine to run but waits for the enable. This basically tells hardware to get ready
pio_sm_set_enabled() - starts the PIO state machine so the hardware 
                       begins executing instructions

