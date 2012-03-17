all: main.s
	avr-gcc -I. -mmcu=atmega88a -Os -x assembler-with-cpp -Wl,--section-start=.text=0x1E00 -DBOARD_ADDR=0x$(BOARD_ADDR) -o bootloader.elf main.s
	avr-objcopy -I elf32-avr -O ihex bootloader.elf bootloader.hex

clean:
	rm -f bootloader.hex bootloader.elf
