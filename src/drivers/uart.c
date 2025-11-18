#include "hardware/uart.h" 
#include "hardware/gpio.h" 
#include <stdio.h>  
#include <string.h>
#include "pico/stdlib.h"
#include "sdcard.h"

#define BUFSIZE 100
char serbuf[BUFSIZE];
int seridx = 0;
int newline_seen = 0;

// add this here so that compiler does not complain about implicit function
// in init_uart_irq
void uart_rx_handler();

/*******************************************************************/

void init_uart()
{
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
    uart_init(uart0, 115200);
    uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
}

void init_uart_irq() 
{
    uart_set_fifo_enabled(uart0, false);
    uart0_hw->imsc = (1u << UART_UARTIMSC_RXIM_LSB) | (1u << UART_UARTIMSC_RTIM_LSB);
    irq_set_exclusive_handler(33, uart_rx_handler);
    irq_set_enabled(33, true);
    // fill in.
}

void uart_rx_handler() 
{
    uart0_hw->icr = (1u << UART_UARTICR_RXIC_LSB) | (1u << UART_UARTICR_RTIC_LSB);

    if(seridx >= BUFSIZ)
    {
        return ;
    }

    char c = (uart0_hw->dr & 0xFF);

    if(c == 0x0A)
    {
        newline_seen = 1;
    }

    if (c == '\b') 
    { 
        if (seridx > 0) 
        {
            uart_putc(uart0, '\b');
            uart_putc(uart0, ' ');
            uart_putc(uart0, '\b');

            seridx--;
            serbuf[seridx] = '\0';
        }
        return;
    }
    else 
    {
        uart_putc(uart0, c); // Echo back typed character
        serbuf[seridx] = c;
        seridx++;
    }
    // fill in.
}

int _read(__unused int handle, char *buffer, int length) 
{
     while(!newline_seen)
    {
        sleep_ms(5);

    }
    newline_seen = 0;

    for(int t = 0; t < seridx; t++)
    {
        buffer[t] = serbuf[t];
    }
    seridx = 0;
    
    return length;
    // fill in.
}

int _write(__unused int handle, char *buffer, int length) 
{
    for(int temp1 = 0; temp1 < length; temp1++)
    {    
        while(!uart_is_writable(uart0))
        {
            tight_loop_contents();
        }
        uart_putc(uart0, buffer[temp1]);
    }
    return length;
    // fill in.
}

/*******************************************************************/

struct commands_t {
    const char *cmd;
    void      (*fn)(int argc, char *argv[]);
};

struct commands_t cmds[] = {
        { "append", append },
        { "cat", cat },
        { "cd", cd },
        { "date", date },
        { "input", input },
        { "ls", ls },
        { "mkdir", mkdir },
        { "mount", mount },
        { "pwd", pwd },
        { "rm", rm },
        { "restart", restart }
};

// This function inserts a string into the input buffer and echoes it to the UART
// but whatever is "typed" by this function can be edited by the user.
void insert_echo_string(const char* str) {
    // Print the string and copy it into serbuf, allowing editing
    seridx = 0;
    newline_seen = 0;
    memset(serbuf, 0, BUFSIZE);

    // Print and fill serbuf with the initial string
    for (int i = 0; str[i] != '\0' && seridx < BUFSIZE - 1; i++) {
        char c = str[i];
        uart_write_blocking(uart0, (uint8_t*)&c, 1);
        serbuf[seridx++] = c;
    }
}

void parse_command(const char* input) {
    char *token = strtok(input, " ");
    int argc = 0;
    char *argv[10];
    while (token != NULL && argc < 10) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    
    int i = 0;
    for(; i<sizeof cmds/sizeof cmds[0]; i++) {
        if (strcmp(cmds[i].cmd, argv[0]) == 0) {
            cmds[i].fn(argc, argv);
            break;
        }
    }
    if (i == (sizeof cmds/sizeof cmds[0])) {
        printf("Unknown command: %s\n", argv[0]);
    }
}

void command_shell() {
    char input[100];
    memset(input, 0, sizeof(input));

    // Disable buffering for stdout
    setbuf(stdout, NULL);

    printf("\nEnter current ");
    insert_echo_string("date 20250701120000");
    fgets(input, 99, stdin);
    input[strcspn(input, "\r")] = 0; // Remove CR character
    input[strcspn(input, "\n")] = 0; // Remove newline character
    parse_command(input);
    
    printf("SD Card Command Shell");
    printf("\r\nType 'mount' to mount the SD card.\n");
    while (1) {
        printf("\r\n> ");
        fgets(input, sizeof(input), stdin);
        fflush(stdin);
        input[strcspn(input, "\r")] = 0; // Remove CR character
        input[strcspn(input, "\n")] = 0; // Remove newline character
        
        parse_command(input);
    }
}