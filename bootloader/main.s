#include <avr/io.h>

#include "hardware_config.h"
#include "mcp_config.h"

.macro SELECT_CAN
	cbi _SFR_IO_ADDR(CAN_PORT), CAN_CS
.endm
.macro DESELECT_CAN
	sbi _SFR_IO_ADDR(CAN_PORT), CAN_CS
.endm

.text

.global main

spi_send:
	out _SFR_IO_ADDR(SPDR), r24
.L1:
	in r18, _SFR_IO_ADDR(SPSR)
	sbrs r18, SPIF
	rjmp .L1
	in r24, _SFR_IO_ADDR(SPDR)
	ret

mcp_get:
	mov r19, r24
	SELECT_CAN
	ldi r24, 0x03
	rcall spi_send
	mov r24, r19
	rcall spi_send
	rcall spi_send
	DESELECT_CAN
	ret

/*
 * r24: addr
 * r22: value
 */
mcp_write:
	mov r19, r24
	SELECT_CAN
	ldi r24, 0x02
	rcall spi_send
	mov r24, r19
	rcall spi_send
	mov r24, r22
	rcall spi_send
	DESELECT_CAN
	ret

wait_can:
	SELECT_CAN
	ldi r24, 0xA0
	rcall spi_send
	rcall spi_send
	DESELECT_CAN
	sbrs r24, 0
	rjmp wait_can
	ret

main:
	// Initialize SPI
	ldi r18, SPI_MASK
	out _SFR_IO_ADDR(SPI_DDR), r18

	ldi r18, _BV(SPE) | _BV(MSTR) | 1
	out _SFR_IO_ADDR(SPCR), r18

	ldi r18, 0x01
	out _SFR_IO_ADDR(SPSR), r18

	// Initialize CAN
	sbi _SFR_IO_ADDR(CAN_DDR), CAN_CS

	// Send RESET
	SELECT_CAN
	ldi r24, 0xC0
	rcall spi_send
	DESELECT_CAN

	// Delay ~10ms. This is for a 20 MHz clock
	ldi r25, 0xFF
	ldi r24, 0xFF
.L2:
	sbiw r24, 1
	brne .L2

	// Simply configure the whole register map
	SELECT_CAN
	ldi r24, 0x02
	rcall spi_send

	ldi r24, 0x00
	rcall spi_send

	ldi r17, 62
	ldi ZH, hi8(mcp_config)
	ldi ZL, lo8(mcp_config)
	
.L3:
	lpm r24, Z+
	rcall spi_send
	dec r17
	brne .L3
	DESELECT_CAN

	// Set normal mode
	ldi r24, 0x0F
	mov r22, r1
	rcall mcp_write

	// Check if we want to enter the bootloader
	ldi r24, 0x0D // TXRTSCTRL
	rcall mcp_get
	mov r30, r1
	mov r31, r1
	sbrc r24, 3
	ijmp

	// Send magic packet
	ldi r24, 0x30
	ldi r22, 0x08
	rcall mcp_write

main_loop:
	// Flush RXB0
	ldi r24, 0x2C
	mov r22, r1
	rcall mcp_write

	rcall wait_can
	SELECT_CAN
	ldi r24, 0x92
	rcall spi_send

	rcall spi_send // MCP will ignore input
	cpi r24, 0x01 // Set Z-Pointer
	breq set_zpointer

	cpi r24, 0x02 // Read flash
	breq read_flash

	cpi r24, 0x03 // Erase flash
	breq erase_flash

	cpi r24, 0x04 // Load buffer
	breq load_flash_buffer

	cpi r24, 0x05 // Write flash
	breq write_flash

	cpi r24, 0x6
	breq reset

main_loop_cleanup:
	DESELECT_CAN
	eor r1, r1
	rjmp main_loop

spm_wait:
	in r19, _SFR_IO_ADDR(SPMCSR)
	sbrc r19, 0
	rjmp spm_wait

	out _SFR_IO_ADDR(SPMCSR), r24
	spm
	ret

/*
 * 01 ZH ZL - Set Z-Pointer
 */
set_zpointer:
	rcall spi_send
	mov ZH, r24
	rcall spi_send
	mov ZL, r24
	rjmp main_loop_cleanup

enable_rww:
	ldi r24, 0x11
	rcall spm_wait
	ret

/*
 * 03 - Erase Flash
 *
 * Uses current Z-Pointer
 */
erase_flash:
	mov r20, ZH
	mov r21, ZL
	ldi r24, 0x03 // Erase
	rcall spm_wait

	rcall enable_rww

	rjmp main_loop_cleanup

/*
 * 04 count d1 .. dN - Load Buffer
 *
 * Uses current Z-Pointer, adds 8
 */
load_flash_buffer:
	rcall spi_send
	mov r17, r24

.L6:
	rcall spi_send
	mov r0, r24
	rcall spi_send
	mov r1, r24

	ldi r24, 0x01 // Load Buffer
	rcall spm_wait

	adiw ZL, 2
	dec r17
	brne .L6

	rjmp main_loop_cleanup

/*
 * 05 - Write flash
 */
write_flash:
	mov ZH, r20
	mov ZL, r21
	ldi r24, 0x05 // Write
	rcall spm_wait

	rcall enable_rww

	rjmp main_loop_cleanup

/*
 * 02 length - Read flash
 *
 * Uses current Z-Pointer, adds length
 */
read_flash:
	rcall spi_send
	mov r17, r24
	ldi r19, 0x36
	add r17, r19

	DESELECT_CAN
.L7:
	lpm r22, Z+
	mov r24, r19
	rcall mcp_write
	inc r19
	cp r19, r17
	brlt .L7

	ldi r24, 0x30
	ldi r22, 0x08
	rcall mcp_write

	rjmp main_loop_cleanup

/*
 * 06 - Reset thyself
 */

reset:
	mov r30, r1
	mov r31, r1
	ijmp
