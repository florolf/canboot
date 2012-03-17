#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/can/raw.h>

#include "can.h"
#include "util.h"
#include "log.h"

int can_create_socket(const char *name) {
	int fd;

	fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(fd < 0) {
		perror("Creating can socket failed");
		return -1;
	}

	struct ifreq ifr;

	strlcpy(ifr.ifr_name, name, IFNAMSIZ);
	if(ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
		perror("Getting interface number failed");
		return -1;
	}

	struct sockaddr_can addr;
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if(bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_can)) < 0) {
		perror("Binding can socket failed");
		return -1;
	}

	return fd;
}

int can_set_filter(int fd, canid_t addr) {
	struct can_filter filter;

	filter.can_id = addr;
	filter.can_mask = CAN_EFF_MASK;

	return setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(struct can_filter));
}

int can_receive_frame(int fd, struct can_frame *msg) {
	ssize_t len;

	len = read(fd, msg, sizeof(struct can_frame));

	if(len < 0) {
		perror("Reading from can socket failed");
		return -1;
	}

	if(len != sizeof(struct can_frame)) {
		log("Received incomplete can frame");
		return -1;
	}

	return 0;
}

static int can_send_frame(int fd, struct can_frame *msg) {
	ssize_t len;

	do {
		len = write(fd, msg, sizeof(struct can_frame));
		usleep(10000);
	} while (len < 0 && errno == ENOBUFS);

	if(len < 0) {
		perror("Writing to can socket failed");
		return -1;
	}

	if(len != sizeof(struct can_frame)) {
		log("Was unable to send whole can frame");
		return -1;
	}

	// Wait a bit for the controller to do its work

	return 0;
}

int can_send_set_zpointer(int fd, canid_t addr, uint16_t pointer) {
	struct can_frame msg;

	msg.can_id = addr | 1 << 31;
	msg.can_dlc = 8;
	msg.data[0] = 0x01; // Command
	msg.data[1] = pointer >> 8; // ZH
	msg.data[2] = pointer & 0xFF; // ZL

	return can_send_frame(fd, &msg);
}

int can_send_erase_flash(int fd, canid_t addr) {
	struct can_frame msg;

	msg.can_id = addr | 1 << 31;
	msg.can_dlc = 8;
	msg.data[0] = 0x03; // Command

	return can_send_frame(fd, &msg);
}

int can_send_reset(int fd, canid_t addr) {
	struct can_frame msg;

	msg.can_id = addr | 1 << 31;
	msg.can_dlc = 8;
	msg.data[0] = 0x06; // Command

	return can_send_frame(fd, &msg);
}

int can_send_load_buffer(int fd, canid_t addr, uint8_t len, uint8_t *data) {
	struct can_frame msg;

	assert(len <= 6);
	assert(len % 2 == 0);

	msg.can_id = addr | 1 << 31;
	msg.can_dlc = 8;
	msg.data[0] = 0x04; // Command
	msg.data[1] = len / 2; // length

	memcpy(&msg.data[2], data, len);

	return can_send_frame(fd, &msg);
}

int can_send_write_flash(int fd, canid_t addr) {
	struct can_frame msg;

	msg.can_id = addr | 1 << 31;
	msg.can_dlc = 8;
	msg.data[0] = 0x05; // Command

	return can_send_frame(fd, &msg);
}

int can_send_read_flash(int fd, canid_t addr, uint8_t len, uint8_t *data) {
	struct can_frame msg;

	assert(len <= 8);

	msg.can_id = addr | 1 << 31;
	msg.can_dlc = 8;
	msg.data[0] = 0x02; // Command
	msg.data[1] = len; // length

	if(can_send_frame(fd, &msg) < 0) {
		log("Sending read request failed");
		return -1;
	}

	if(can_receive_frame(fd, &msg) < 0) {
		log("Receiving read answer failed");
		return -1;
	}

	memcpy(data, msg.data, len);

	return 0;
}
