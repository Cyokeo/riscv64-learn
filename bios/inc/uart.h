// uart.h - UART driver header for RISC-V64
#ifndef __BIOS_UART_H__
#define __BIOS_UART_H__

// Standard includes for embedded environment
#include <stdarg.h>

// Configuration
#define UART_BUFFER_SIZE    256     // Input buffer size
#define UART_DEFAULT_BAUD   115200  // Default baud rate

// UART statistics structure
typedef struct {
    unsigned int bytes_received;
    unsigned int bytes_transmitted;
    unsigned int lines_processed;
} uart_stats_t;

// Function prototypes

// Initialization
void uart_init(unsigned int baud_rate);

// Basic I/O functions
void uart_putc(char c);
void uart_puts(const char* str);
void uart_println(const char* str);

// Formatted output
void uart_printf(const char* format, ...);
void uart_print_int(int num);
void uart_print_hex(unsigned int num);

// Interrupt handler (called from assembly)
void uart_interrupt_handler(void);

// Statistics
uart_stats_t uart_get_stats(void);

// Utility functions
int strcmp(const char* s1, const char* s2);

// System functions (implemented in assembly)
extern void system_reboot(void);

// Internal functions (static in C file)
static void handle_receive_interrupt(void);
static void handle_line_status_interrupt(void);
static void process_command(const char* cmd);
static void handle_backspace(void);

#endif /* __BIOS_UART_H__ */