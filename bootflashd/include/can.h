#ifndef _CAN_H_
#define _CAN_H_

#include <sys/socket.h>
#include <linux/can.h>

int can_create_socket(const char *);
int can_set_filter(int fd, canid_t addr);
int can_receive_frame(int fd, struct can_frame *msg);
int can_send_set_zpointer(int fd, canid_t addr, uint16_t pointer);
int can_send_erase_flash(int fd, canid_t addr);
int can_send_reset(int fd, canid_t addr);
int can_send_load_buffer(int fd, canid_t addr, uint8_t len, uint8_t *data);
int can_send_write_flash(int fd, canid_t addr);
int can_send_read_flash(int fd, canid_t addr, uint8_t len, uint8_t *data);

#endif /* _CAN_H_ */
