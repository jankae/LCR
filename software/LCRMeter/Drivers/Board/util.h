#ifndef UTIL_H_
#define UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define ARRAY_SIZE(a)		(sizeof(a)/(sizeof(a[0])))

static inline int32_t util_sign_extend_32(int32_t x, uint16_t bits) {
	int32_t m = 1 << (bits - 1);
	return (x ^ m) - m;
}

static inline char util_nibble_to_hex_char(uint8_t n) {
	return n < 10 ? n + '0' : n + 'A' - 0x0A;
}

static inline uint8_t util_hex_to_nibble(char c) {
	return c <= '9' ? c - '0' : c - 'A' + 0x0A;
}

// returns date/time in unix timestamp
uint32_t unixtime(int year, int month, int day,
              int hour, int minute, int sec);

static inline uint16_t htons(uint16_t hostshort) {
	return ((hostshort & 0xFF00) >> 8) | ((hostshort & 0x00FF) << 8);
}

static inline uint16_t ntohs(uint16_t netshort) {
	return ((netshort & 0xFF00) >> 8) | ((netshort & 0x00FF) << 8);
}

static inline uint32_t htonl(uint32_t hostlong) {
	return ((hostlong & 0xFF000000) >> 24) | ((hostlong & 0x00FF0000) >> 8)
			| ((hostlong & 0x0000FF00) << 8) | ((hostlong & 0x000000FF) << 24);
}

static inline uint32_t ntohl(uint32_t netlong) {
	return ((netlong & 0xFF000000) >> 24) | ((netlong & 0x00FF0000) >> 8)
			| ((netlong & 0x0000FF00) << 8) | ((netlong & 0x000000FF) << 24);
}

static inline int16_t constrain_int16_t(int16_t val, int16_t min, int16_t max) {
	if(val < min) {
		return min;
	} else if(val > max) {
		return max;
	} else {
		return val;
	}
}

int32_t util_Map(int32_t value, int32_t scaleFromLow, int32_t scaleFromHigh,
        int32_t scaleToLow, int32_t scaleToHigh);

uint32_t util_crc32(uint32_t crc, const void *data, uint32_t len);

typedef struct coords {
	int16_t x;
	int16_t y;
} coords_t;

#ifdef __cplusplus
}
#endif

#endif
