// uart.c - UART driver implementation for RISC-V64
#include "uart.h"

// UART register definitions
#define UART_BASE       0x10000000UL
#define UART_THR        0x00    // Transmit Holding Register
#define UART_RBR        0x00    // Receive Buffer Register  
#define UART_IER        0x01    // Interrupt Enable Register
#define UART_IIR        0x02    // Interrupt Identification Register
#define UART_FCR        0x02    // FIFO Control Register
#define UART_LCR        0x03    // Line Control Register
#define UART_MCR        0x04    // Modem Control Register
#define UART_LSR        0x05    // Line Status Register
#define UART_DLL        0x00    // Divisor Latch Low (when DLAB=1)
#define UART_DLH        0x01    // Divisor Latch High (when DLAB=1)

// UART register access macros
#define UART_REG(offset) (*((volatile unsigned char*)(UART_BASE + (offset))))

// Input buffer for line editing
static char input_buffer[UART_BUFFER_SIZE];
static int buffer_pos = 0;

// Statistics
static uart_stats_t stats = {0};

// Initialize UART with specified baud rate
void uart_init(unsigned int baud_rate) {
    // Calculate divisor for baud rate
    // Assuming 22.729 MHz clock (typical for QEMU virt)
    unsigned int divisor = 22729000 / (16 * baud_rate);
    
    // Set DLAB bit to access divisor registers
    UART_REG(UART_LCR) = 0x80;
    
    // Set baud rate divisor
    UART_REG(UART_DLL) = divisor & 0xFF;
    UART_REG(UART_DLH) = (divisor >> 8) & 0xFF;
    
    // Configure line: 8N1 (8 bits, no parity, 1 stop bit)
    UART_REG(UART_LCR) = 0x03;
    
    // Enable FIFO with 4-byte trigger level
    UART_REG(UART_FCR) = 0x07;
    
    // Enable receive and line status interrupts
    UART_REG(UART_IER) = 0x05;
    
    // Reset statistics
    stats.bytes_received = 0;
    stats.bytes_transmitted = 0;
    stats.lines_processed = 0;
}

// Send a single character (blocking)
void uart_putc(char c) {
    // Wait for transmitter to be ready
    while (!(UART_REG(UART_LSR) & 0x20));
    
    // Send character
    UART_REG(UART_THR) = c;
    stats.bytes_transmitted++;
}

// Send a string
void uart_puts(const char* str) {
    while (*str) {
        uart_putc(*str++);
    }
}

// Send a string with newline
void uart_println(const char* str) {
    uart_puts(str);
    uart_putc('\r');
    uart_putc('\n');
}

// Print formatted string (simple printf implementation)
void uart_printf(const char* format, ...) {
    // Simple implementation - only supports %s, %d, %x, %c
    const char* p = format;
    va_list args;
    va_start(args, format);
    
    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++;
            switch (*p) {
                case 's': {
                    const char* str = va_arg(args, const char*);
                    uart_puts(str ? str : "(null)");
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    uart_print_int(num);
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    uart_print_hex(num);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    uart_putc(c);
                    break;
                }
                case '%':
                    uart_putc('%');
                    break;
                default:
                    uart_putc('%');
                    uart_putc(*p);
                    break;
            }
        } else {
            uart_putc(*p);
        }
        p++;
    }
    
    va_end(args);
}

// Print integer
void uart_print_int(int num) {
    if (num == 0) {
        uart_putc('0');
        return;
    }
    
    if (num < 0) {
        uart_putc('-');
        num = -num;
    }
    
    char buffer[12]; // Enough for 32-bit int
    int pos = 0;
    
    while (num > 0) {
        buffer[pos++] = '0' + (num % 10);
        num /= 10;
    }
    
    // Print digits in reverse order
    while (pos > 0) {
        uart_putc(buffer[--pos]);
    }
}

// Print hexadecimal number
void uart_print_hex(unsigned int num) {
    uart_puts("0x");
    
    if (num == 0) {
        uart_putc('0');
        return;
    }
    
    char buffer[9]; // 8 hex digits + null
    int pos = 0;
    
    while (num > 0) {
        int digit = num & 0xF;
        buffer[pos++] = digit < 10 ? '0' + digit : 'A' + digit - 10;
        num >>= 4;
    }
    
    // Print digits in reverse order
    while (pos > 0) {
        uart_putc(buffer[--pos]);
    }
}

// Process a complete command line
static void process_command(const char* cmd) {
    if (cmd[0] == '\0') {
        return; // Empty command
    }
    
    // Simple command processing
    if (strcmp(cmd, "help") == 0) {
        uart_println("Available commands:");
        uart_println("  help     - Show this help");
        uart_println("  stats    - Show UART statistics");
        uart_println("  clear    - Clear screen");
        uart_println("  echo     - Echo test");
        uart_println("  reboot   - Restart system");
    }
    else if (strcmp(cmd, "stats") == 0) {
        uart_printf("UART Statistics:\r\n");
        uart_printf("  Bytes received: %d\r\n", stats.bytes_received);
        uart_printf("  Bytes transmitted: %d\r\n", stats.bytes_transmitted);
        uart_printf("  Lines processed: %d\r\n", stats.lines_processed);
    }
    else if (strcmp(cmd, "clear") == 0) {
        // Send ANSI clear screen sequence
        uart_puts("\033[2J\033[H");
    }
    else if (strcmp(cmd, "echo") == 0) {
        uart_println("Echo test - type something:");
        // Echo mode - just continue normal echo behavior
    }
    else if (strcmp(cmd, "reboot") == 0) {
        uart_println("Rebooting system...");
        // In a real system, this would trigger a reset
        // For now, just show message
        system_reboot();
    }
    else {
        uart_printf("Unknown command: %s\r\n", cmd);
        uart_println("Type 'help' for available commands.");
    }
}

// Handle backspace/delete
static void handle_backspace(void) {
    if (buffer_pos > 0) {
        buffer_pos--;
        // Send backspace sequence: backspace, space, backspace
        uart_putc('\b');
        uart_putc(' ');
        uart_putc('\b');
    }
}

// UART interrupt handler (called from assembly)
void uart_interrupt_handler(void) {
    unsigned char iir = UART_REG(UART_IIR);
    
    // Check interrupt type
    switch (iir & 0x0F) {
        case 0x04: // Received Data Available
        case 0x0C: // Character Timeout
            handle_receive_interrupt();
            break;
            
        case 0x02: // Transmitter Holding Register Empty
            // Handle transmit interrupt if needed
            break;
            
        case 0x06: // Receiver Line Status
            // Handle line status errors
            handle_line_status_interrupt();
            break;
            
        default:
            break;
    }
}

// Handle receive interrupt
static void handle_receive_interrupt(void) {
    while (UART_REG(UART_LSR) & 0x01) { // Data available
        char c = UART_REG(UART_RBR);
        stats.bytes_received++;
        
        // Handle special characters
        switch (c) {
            case '\r': // Carriage return
            case '\n': // Line feed
                uart_putc('\r');
                uart_putc('\n');
                
                // Process the command
                input_buffer[buffer_pos] = '\0';
                process_command(input_buffer);
                stats.lines_processed++;
                
                // Reset buffer and show prompt
                buffer_pos = 0;
                uart_puts("BIOS> ");
                break;
                
            case '\b': // Backspace
            case 0x7F: // Delete
                handle_backspace();
                break;
                
            case 0x03: // Ctrl+C
                uart_println("^C");
                buffer_pos = 0;
                uart_puts("BIOS> ");
                break;
                
            case 0x04: // Ctrl+D (EOF)
                uart_println("Goodbye!");
                // Could implement shutdown here
                break;
                
            default:
                // Printable character
                if (c >= 0x20 && c <= 0x7E) {
                    if (buffer_pos < UART_BUFFER_SIZE - 1) {
                        input_buffer[buffer_pos++] = c;
                        uart_putc(c); // Echo character
                    } else {
                        uart_putc('\a'); // Bell - buffer full
                    }
                }
                break;
        }
    }
}

// Handle line status interrupt (errors)
static void handle_line_status_interrupt(void) {
    unsigned char lsr = UART_REG(UART_LSR);
    
    if (lsr & 0x02) {
        uart_println("UART: Overrun error");
    }
    if (lsr & 0x04) {
        uart_println("UART: Parity error");
    }
    if (lsr & 0x08) {
        uart_println("UART: Framing error");
    }
    if (lsr & 0x10) {
        uart_println("UART: Break interrupt");
    }
}

// Get UART statistics
uart_stats_t uart_get_stats(void) {
    return stats;
}

// Simple string comparison
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// System reboot function (to be implemented in assembly)
extern void system_reboot(void);