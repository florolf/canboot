#ifndef _FIRMWARE_H_
#define _FIRMWARE_H_

#include "can.h"

int firmware_flash(int cansock, const char *firmware_dir, uint32_t addr);

#endif /* _FIRMWARE_H_ */
