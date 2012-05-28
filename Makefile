DEVICE     = atmega8535
CLOCK      = 8000000
PROGRAMMER = -c avr910 -P /dev/ttyACM0
FUSES      = -U hfuse:w:0xd9:m -U lfuse:w:0x24:m

CC         = avr-gcc
CFLAGS     = -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)
LDFLAGS    = 
SOURCES    = main.c

# Tune the lines below only if you know what you are doing:

COMPILE    = $(CC) $(CFLAGS) $(LDFLAGS)
OBJECTS    = $(SOURCES:.c=.o)
ELFS       = $(SOURCES:.c=.elf)
HEXADECIMAL= $(SOURCES:.c=.hex)

AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE)
COMPILE = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

# symbolic targets:
all:    $(HEXADECIMAL)

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(COMPILE) -S $< -o $@

flash:  all
	$(AVRDUDE) -U flash:w:main.hex:i

fuse:
	$(AVRDUDE) $(FUSES)

# Xcode uses the Makefile targets "", "clean" and "install"
rebuild: clean all install

install: flash fuse

# if you use a bootloader, change the command below appropriately:
load: all
	bootloadHID $(HEXADECIMAL)

clean:
	rm -f $(HEXADECIMAL) $(ELFS) $(OBJECTS)

erase:
	$(AVRDUDE) -e

# file targets:
$(ELFS): $(OBJECTS)
	$(COMPILE) -o $(ELFS) $(OBJECTS)

$(HEXADECIMAL): $(ELFS)
	rm -f $(HEXADECIMAL)
	avr-objcopy -j .text -j .data -O ihex $(ELFS) $(HEXADECIMAL)
# If you have an EEPROM section, you must also create a hex file for the
# EEPROM and add it to the "flash" target.

# Targets for code debugging and analysis:
disasm: $(ELFS)
	avr-objdump -d $(ELFS)

cpp:
	$(COMPILE) -E $(SOURCES)

