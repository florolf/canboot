#include "util.h"

size_t strlcpy(char *dst, const char *src, size_t len) {
	size_t cnt = 0;

	while(cnt + 1 < len && *src) {
		*dst++ = *src++;
		cnt++;
	}

	if(len)
		*dst = 0;

	return cnt + strlen(src);
}

void chomp(char *str) {
	while(*str++)
		if(*str == '\r' || *str == '\n')
			*str = 0;
}
