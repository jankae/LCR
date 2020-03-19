#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

#define COORDS(v1, v2)	((coords_t){(int16_t) (v1), (int16_t) (v2)})
#define SIZE(v1, v2)	COORDS(v1, v2)

const coords_t operator+(coords_t const& lhs, coords_t const& rhs);

namespace Unit {

using unit = struct unit {
	/* name length up to 3 (plus string terminator) */
	char name[4];
	uint32_t factor;
};

//typedef const unitElement_t *Unit::unit[];

extern const unit *Current[], *Voltage[], *Power[], *Temperature[],
		*Resistance[], *Energy[], *Time[], *Memory[], *Capacity[], *Percent[],
		*Charge[], *Weight[], *Force[], *None[], *Hex[], *Frequency[], *Distance[];
extern const int32_t null, maxPercent;

uint32_t LeastDigitValueFromString(const char *s,
		const unit *unit[]);

void StringFromValue(char *to, uint8_t len, int32_t val,
		const unit *unit[]);

uint8_t ValueFromString(int32_t *value, char *s, const unit *unit[]);
int32_t ValueFromString(const char *s, uint32_t multiplier);

}

#endif
