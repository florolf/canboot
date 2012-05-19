OBJS_BOOTFLASHD=src/main.o \
		src/can.o \
		src/util.o \
		src/firmware.o

OBJS_DUMPFW=src/can.o \
	    src/util.o \
	    src/dumpfw.o

GIT_REV=$(shell git rev-parse HEAD)

all: bootflashd dumpfw

dumpfw: $(OBJS_DUMPFW)
	gcc -g -o dumpfw $(OBJS_DUMPFW)

bootflashd: $(OBJS_BOOTFLASHD)
	gcc -g -o bootflashd $(OBJS_BOOTFLASHD)

clean:
	rm -f $(OBJS_BOOTFLASHD) $(OBJS_DUMPFW) bootflashd dumpfw

%.o: %.c
	gcc -g -Wall -Werror -DGIT_REV=\"$(GIT_REV)\" -std=gnu99 -Iinclude/ -c -o $@ $<
