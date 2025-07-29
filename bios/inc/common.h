#ifndef __BIOS_COMMON_H__
#define __BIOS_COMMON_H__
// Memory map for QEMU virt machine
#define UART_BASE     (0x10000000U)
#define UART_THR      (0x00U)    // Transmit Holding Register
#define UART_RBR      (0x00U)    // Receive Buffer Register  
#define UART_IER      (0x01U)    // Interrupt Enable Register
#define UART_IIR      (0x02U)    // Interrupt Identification Register
#define UART_FCR      (0x02U)    // FIFO Control Register
#define UART_LCR      (0x03U)    // Line Control Register
#define UART_MCR      (0x04U)    // Modem Control Register
#define UART_LSR      (0x05U)    // Line Status Register

#define PLIC_BASE      (0x0C000000U)
#define PLIC_PRIORITY  (0x0C000000U)
#define PLIC_PENDING   (0x0C001000U)
#define PLIC_ENABLE    (0x0C002000U)
#define PLIC_THRESHOLD (0x0C200000U)
#define PLIC_CLAIM     (0x0C200004U)

#endif /* __BIOS_COMMON_H__ */