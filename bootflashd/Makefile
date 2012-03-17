OBJS=src/main.o \
     src/can.o \
     src/util.o \
     src/firmware.o

GIT_REV=$(shell git rev-parse HEAD)

all: $(OBJS)
	gcc -g -o bootflashd $(OBJS)

clean:
	rm -f $(OBJS) bootflashd

%.o: %.c
	gcc -g -Wall -Werror -DGIT_REV=\"$(GIT_REV)\" -std=gnu99 -Iinclude/ -c -o $@ $<
