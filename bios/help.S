# Assembly helper functions for C code

# Function: uart_puts (optimized assembly version)
.global uart_puts
uart_puts:
    li t0, UART_BASE
    mv t1, a0                  # String pointer
uart_puts_loop:
    lb t2, 0(t1)               # Load character
    beqz t2, uart_puts_done    # Check for null terminator
    
    # Wait for transmitter ready
uart_puts_wait:
    lb t3, 5(t0)               # Read LSR
    andi t3, t3, 0x20          # Check THRE bit
    beqz t3, uart_puts_wait
    
    # Send character
    sb t2, 0(t0)               # Write to THR
    
    addi t1, t1, 1             # Next character
    j uart_puts_loop
    
uart_puts_done:
    ret

# Function: uart_println (puts + newline)
.global uart_println
uart_println:
    addi sp, sp, -16
    sd ra, 0(sp)
    
    call uart_puts             # Print string
    
    # Print \r\n
    li t0, UART_BASE
    li t2, 13                  # \r
    
uart_println_cr_wait:
    lb t3, 5(t0)
    andi t3, t3, 0x20
    beqz t3, uart_println_cr_wait
    sb t2, 0(t0)
    
    li t2, 10                  # \n
uart_println_lf_wait:
    lb t3, 5(t0)
    andi t3, t3, 0x20
    beqz t3, uart_println_lf_wait
    sb t2, 0(t0)
    
    ld ra, 0(sp)
    addi sp, sp, 16
    ret
