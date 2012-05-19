#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "firmware.h"

#include "log.h"
#include "can.h"
#include "util.h"

struct controller_param {
	const char *name;

	uint32_t bootloader_offset;
	uint32_t pagesize;
} controller_params[] = {
	{"atmega88", 0x1E00, 64},
	{NULL, 0, 0}
};

static const char *firmware_make_path(const char *firmware_dir, uint32_t addr) {
	static char path_out[256];

	snprintf(path_out, 256, "%s/%08X.hex", firmware_dir, addr);

	return path_out;
}

enum ihex_record {
	IHEX_INVALID = 0xFF,
	IHEX_DATA = 0x00,
	IHEX_EOF = 0x01
};

#define HEX2DEC(x) (((x) <= '9') ? ((x) - '0') : (10 + ((x) | 0x20) - 'a'))

static enum ihex_record parse_ihex(const char *line, uint32_t *offset, uint32_t *length, uint8_t **data) {
	enum ihex_record type;

	if(sscanf(line, ":%02X%04X%02X", length, offset, &type) != 3) {
		log("Got invalid IHEX record \"%s\"", line);
		return IHEX_INVALID;
	}

	if(type == IHEX_DATA) {
		const char *p = line + 9;
		if(strlen(p) - 2 < *length * 2) {
			log("Record too short");
			return IHEX_INVALID;
		}

		if(data) {
			*data = malloc(*length);

			for(int i=0; i < *length; i++) {
				(*data)[i] = HEX2DEC(p[2*i]) << 4 |
					HEX2DEC(p[2*i+1]);
			}
		}
	}

	return type;
}

#define PAGE_ROUND_DOWN(pagesize, addr) (((addr)/(pagesize))*(pagesize))

static int firmware_validate(const char *firmware_path, struct controller_param **params) {
	FILE *f = fopen(firmware_path, "r");

	if(!f) {
		perror("Opening firmware file failed");
		return 0;
	}

	int ret = 0;
	char buf[256];
	uint32_t offset = 0;
	bool got_type = false;

	while(fgets(buf, 256, f)) {
		chomp(buf);

		if(buf[0] == '#')
			continue;
		else if(buf[0] == 'C') {
			const char *name = &buf[2];
			size_t name_len = strlen(name);

			got_type = true;

			struct controller_param *p;

			*params = NULL;
			for(p = controller_params; p->name; p++)
				if(!strncasecmp(p->name, name, name_len)) {
					*params = p;
					break;
				}

			if(!*params) {
				log("Unknown controller type \"%s\"", name);
				ret = 0;
				goto out;
			}
		}
		else if(buf[0] == ':') {
			if(!got_type) {
				log("Got IHEX before type statement");
				ret = 0;
				goto out;
			}

			uint32_t new_offset, length;

			enum ihex_record rtype = parse_ihex(buf, &new_offset, &length, NULL);
			switch(rtype) {
				case IHEX_INVALID:
					ret = 0;
					goto out;
					break;

				case IHEX_DATA:
					if(new_offset < offset) {
						log("IHEX is not monotonic");
						ret = 0;
						goto out;
					}

					offset = new_offset;

					if(offset + length > PAGE_ROUND_DOWN((*params)->pagesize, (*params)->bootloader_offset)) {
						log("Data reaches into bootloader area");
						ret = 0;
						goto out;
					}
					break;

				case IHEX_EOF:
					break;

				default:
					log("Unsupported IHEX record %02X", rtype);
					ret = 0;
					goto out;
					break;
			}
		} else {
			log("Got invalid line: %s", buf);
			ret = 0;
			goto out;
		}
	}

	if(!got_type) {
		log("Controller type needs to be specified");
		ret = 0;
		goto out;
	}

	ret = 1;
out:
	fclose(f);
	return ret;
}

#define MIN(a,b) (((a)<(b))?(a):(b))

static int do_flash(int cansock, canid_t addr, uint32_t length, uint8_t *pagebuf) {

	can_send_erase_flash(cansock, addr);

	uint8_t *rest = pagebuf;
	uint32_t rest_len = length;
	while(rest_len) {
		uint8_t send_len = MIN(rest_len, 6);

		can_send_load_buffer(cansock, addr, send_len, rest);

		rest += send_len;
		rest_len -= send_len;
	}

	can_send_write_flash(cansock, addr);

	rest = pagebuf;
	rest_len = length;
	while(rest_len) {
		uint8_t recv_len = MIN(rest_len, 8);
		uint8_t recv_buf[8];

		can_send_read_flash(cansock, addr, recv_len, recv_buf);

		if(memcmp(recv_buf, rest, recv_len)) {
			log("Validating flashed page failed");
			return -1;
		}

		rest += recv_len;
		rest_len -= recv_len;
	}

	return 0;
}

int firmware_flash(int cansock, const char *firmware_dir, canid_t addr) {
	const char *file = firmware_make_path(firmware_dir, addr);

	log("Using firmware file %s", file);

	struct controller_param *params;
	if(!firmware_validate(file, &params)) {
		log("Firmware file validation failed, sending reset");

		can_send_reset(cansock, addr);

		return -1;
	}

	FILE *f = fopen(file, "r");
	char buf[256];
	uint8_t *pagebuf = malloc(params->pagesize);
	uint32_t fillcnt = 0, base_addr;

	while(fgets(buf, 256, f)) {
		chomp(buf);

		if(buf[0] != ':')
			continue;

		uint32_t offset, length;
		uint8_t *data;
		if(parse_ihex(buf, &offset, &length, &data) == IHEX_EOF)
			break;

		while(length) {
			if(fillcnt == 0) { // We're starting a new page
				base_addr = PAGE_ROUND_DOWN(params->pagesize, offset);

				memset(pagebuf, 0, offset - base_addr);
				fillcnt = offset - base_addr;

				can_send_set_zpointer(cansock, addr, base_addr);
			}

			uint32_t to_write = MIN(length, params->pagesize - fillcnt);

			memcpy(pagebuf + fillcnt, data, to_write);
			fillcnt += to_write;
			offset += to_write;
			length -= to_write;

			if(fillcnt == params->pagesize) { // page complete, flash it
				if(do_flash(cansock, addr, fillcnt, pagebuf) < 0) {
					log("Flashing page 0x%X failed", base_addr / 2);

					free(data);
					free(pagebuf);

					return -1;
				}
				fillcnt = 0;
			}

			memmove(data, data + to_write, length);
		}

		free(data);
	}

	if(fillcnt) { // there's some data left
		if(do_flash(cansock, addr, fillcnt, pagebuf) < 0) {
			log("Flashing page 0x%X failed", base_addr / 2);

			free(pagebuf);

			return -1;
		}
	}

	free(pagebuf);

	log("Resetting device");
	can_send_reset(cansock, addr);

	return 0;
}
