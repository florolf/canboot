#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "can.h"
#include "log.h"
#include "firmware.h"

int main(int argc, char **argv) {
	int cansock;
	
	if(argc < 3) {
		die("%s can_interface firware_path", argv[0]);
	}

	log("bootflashd (rev " GIT_REV ") starting up");

	cansock = can_create_socket(argv[1]);
	if(cansock < 0) {
		die("Failed to attach to can interface");
	}

	const char *firmware_dir = argv[2];

	struct can_frame msg;
	while(can_receive_frame(cansock, &msg) >= 0) {
		if(!(msg.can_id & 1 << 31) || // No extended id
		    (msg.can_id & 1 << 30))
			continue;

		if(memcmp(msg.data, "BOOTMAGC", 8))
			continue;

		uint32_t addr = msg.can_id & CAN_EFF_MASK;

		log("Received boot request from %08X", addr);

		int local_sock = can_create_socket(argv[1]);
		can_set_filter(local_sock, addr);

		if(firmware_flash(local_sock, firmware_dir, addr) < 0) {
			log("Flashing firmware failed");

			goto out;
		}

		log("Flashing succeeded");

out:
		close(local_sock);
	}
}
