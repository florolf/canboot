#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "can.h"
#include "log.h"

int main(int argc, char **argv) {
	if(argc < 5) {
		die("%s can_interface addr size file", argv[0]);
	}

	int cansock = can_create_socket(argv[1]);
	if(cansock < 0) {
		die("Failed to attach to can interface");
	}

	canid_t addr;
	sscanf(argv[2], "%x", &addr);
	addr |= CAN_EFF_FLAG;

	can_set_filter(cansock, addr);

	FILE *dump = fopen(argv[4], "w");
	if(!dump)
		die("Opening dump file failed: %s", strerror(errno));

	if(can_send_set_zpointer(cansock, addr, 0) < 0)
		die("Initializing Z-pointer failed");

	size_t length = atoi(argv[3]);
	while(length) {
		uint8_t buf[8];
		size_t read_length = (length >= 8)?8:length;

		if(can_send_read_flash(cansock, addr, read_length, buf) < 0)
			die("Reading flash failed");

		if(fwrite(buf, read_length, 1, dump) != 1)
			die("Writing data to file failed: %s", strerror(errno));

		length -= read_length;
	}

	fclose(dump);
	close(cansock);
}
