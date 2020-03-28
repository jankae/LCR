#include <ctype.h>
#include "Unit.hpp"
#include <cstdio>

static const Unit::unit uA = { "uA", 1 };
static const Unit::unit mA = { "mA", 1000 };
static const Unit::unit A = { "A", 1000000 };

static const Unit::unit uV = { "uV", 1 };
static const Unit::unit mV = { "mV", 1000 };
static const Unit::unit V = { "V", 1000000 };

static const Unit::unit uW = { "uW", 1 };
static const Unit::unit mW = { "mW", 1000 };
static const Unit::unit W = { "W", 1000000 };

static const Unit::unit C = {"\xF8""C", 1};

static const Unit::unit uR = {"uR", 1};
static const Unit::unit mR = {"mR", 1000};
static const Unit::unit R = {"R", 1000000};

static const Unit::unit uWh = {"uWh", 1};
static const Unit::unit mWh = {"mWh", 1000};
static const Unit::unit Wh = {"Wh", 1000000};

static const Unit::unit us = {"us", 1};
static const Unit::unit ms = {"ms", 1000};
static const Unit::unit s = {"s", 1000000};
static const Unit::unit min = {"m", 60000000};

static const Unit::unit B = {"B", 1};
static const Unit::unit kB = {"kB", 1024};

static const Unit::unit uF = {"uF", 1};
static const Unit::unit mF = {"mF", 1000};
static const Unit::unit F = {"F", 1000000};

static const Unit::unit percent = {"%", 1000000};

static const Unit::unit uAh = {"uAh", 1};
static const Unit::unit mAh = {"mAh", 1000};
static const Unit::unit Ah = {"Ah", 1000000};

static const Unit::unit mg = {"mg", 1};
static const Unit::unit g=  {"g", 1000};
static const Unit::unit kg = {"kg", 1000000};

static const Unit::unit uN = {"uN", 1};
static const Unit::unit mN =  {"mN", 1000};
static const Unit::unit N = {"N", 1000000};

static const Unit::unit Hz = {"Hz", 1};
static const Unit::unit kHz =  {"kHz", 1000};

static const Unit::unit mm = {"mm", 1};
static const Unit::unit cm = {"cm", 10};
static const Unit::unit m =  {"m", 1000};

static const Unit::unit none = {"", 1};

const Unit::unit *Unit::Current[] = { &uA, &mA, &A, nullptr };
const Unit::unit *Unit::Voltage[] = { &uV, &mV, &V, nullptr };
const Unit::unit *Unit::Power[] = { &uW, &mW, &W, nullptr };
const Unit::unit *Unit::Temperature[] = {&C, nullptr };
const Unit::unit *Unit::Resistance[] = { &uR, &mR, &R, nullptr };
const Unit::unit *Unit::Energy[] = { &uWh, &mWh, &Wh, nullptr };
const Unit::unit *Unit::Time[] = {&us, &ms, &s, nullptr };
const Unit::unit *Unit::Memory[] = { &B, &kB, nullptr };
const Unit::unit *Unit::Capacity[] = { &uF, &mF, &F, nullptr };
const Unit::unit *Unit::Percent[] = { &percent, nullptr };
const Unit::unit *Unit::Charge[] = { &uAh, &mAh, &Ah, nullptr };
const Unit::unit *Unit::Weight[] = { &mg, &g, &kg, nullptr };
const Unit::unit *Unit::Force[] = { &uN, &mN, &N, nullptr };
const Unit::unit *Unit::None[] = {&none, nullptr };
const Unit::unit *Unit::Hex[] = {nullptr };
const Unit::unit *Unit::Frequency[] = { &Hz, &kHz, nullptr };
const Unit::unit *Unit::Distance[] = {&mm, &m, nullptr };

const int32_t Unit::null = 0;
const int32_t Unit::maxPercent = 100000000;

const coords_t operator+(coords_t const& lhs, coords_t const& rhs) {
	coords_t ret = lhs;
	ret.x += rhs.x;
	ret.y += rhs.y;
	return ret;
}

uint32_t Unit::LeastDigitValueFromString(const char *s,
		const Unit::unit *unit[]) {
	uint32_t dotdivisor = 0;
	while (*s) {
		if (*s == '.') {
			dotdivisor = 1;
		} else if (*s == '-' || *s == ' ' || isdigit((uint8_t )*s)) {
			dotdivisor *= 10;
		} else {
			/* end of value string */
			break;
		}
		s++;
	}
	if(!dotdivisor)
		dotdivisor = 1;
	if (*s) {
		/* try to find matching unit */
		while (*unit) {
			if (!strncmp((*unit)->name, s, strlen((*unit)->name))) {
				/* this unit matches the value string */
				return (*unit)->factor / dotdivisor;
			}
			unit++;
		}
		/* no matching unit found */
		return 0;
	} else {
		/* string ended before detecting unit */
		return 1;
	}
}

void Unit::StringFromValue(char *to, uint8_t len, int32_t val,
		const Unit::unit *unit[]) {
	if (unit == Unit::Hex) {
		uint8_t encodedLength = snprintf(to, len + 1, "0x%X", val);
		if (encodedLength > len) {
			memset(to, '-', len);
			to[len] = 0;
		}
		return;
	}
	/* store sign */
	int8_t negative = 0;
	if (val < 0) {
		val = -val;
		negative = 1;
	}
	/* find applicable unit */
	const Unit::unit *selectedUnit = *unit;
	while (*unit) {
		if (val / (*unit)->factor) {
			/* this unit is a better fit */
			selectedUnit = *unit;
		}
		unit++;
	}
	if (!selectedUnit) {
		/* this should not be possible */
		*to = 0;
		return;
	}
	/* calculate number of available digits */
	uint8_t digits = len - strlen(selectedUnit->name) - negative;
	/* calculate digits before the dot */
	uint32_t intval = val / selectedUnit->factor;
	uint8_t beforeDot = 0;
	while (intval) {
		intval /= 10;
		beforeDot++;
	}
	if (beforeDot > digits) {
		/* value does not fit available space */
		*to = 0;
		return;
	}
	if (!beforeDot)
		beforeDot = 1;
	int8_t spaceAfter = digits - 1 - beforeDot;
	uint32_t factor = 1;
	int8_t afterDot = 0;
	while (spaceAfter > 0 && factor < selectedUnit->factor) {
		factor *= 10;
		spaceAfter--;
		afterDot++;
	}
	val = ((uint64_t) val * factor) / selectedUnit->factor;

	/* compose string from the end */
	/* copy unit */
	uint8_t pos = digits + negative;
	strcpy(&to[pos], selectedUnit->name);
	/* actually displayed digits */
	digits = beforeDot + afterDot;
	while (digits) {
		afterDot--;
		digits--;
		to[--pos] = val % 10 + '0';
		val /= 10;
		if (afterDot == 0) {
			/* place dot at this position */
			to[--pos] = '.';
		}
	}
	if (negative) {
		to[--pos] = '-';
	}
	while (pos > 0) {
		to[--pos] = ' ';
	}
}
bool Unit::SIStringFromFloat(char* to, uint8_t len, float val, char smallest_prefix) {
	if (len < 7) {
		// need at least 7 characters (sign + 4 digits + decimal point and prefix)
		return false;
	}
	using Prefix = struct {
		char name;
		float factor;
	};
	static constexpr Prefix prefixes[] = {
			{'f', 1E-15f},
			{'p', 1E-12f},
			{'n', 1E-9f},
			{'u', 1E-6f},
			{'m', 1E-3f},
			{' ', 1E-0f},
			{'k', 1E+3f},
			{'M', 1E+6f},
			{'G', 1E+9f},
			{'T', 1E+12f},
			{'P', 1E+15f},
	};
	if(val < 0) {
		*to++ = '-';
		val = -val;
	} else {
		*to++ = ' ';
	}
	uint8_t prefix_index = 0;
	while (prefixes[prefix_index].name != smallest_prefix) {
		prefix_index++;
		if (prefix_index > ARRAY_SIZE(prefixes)) {
			// Requested smallest prefix not available
			return false;
		}
	}
	while (abs(val) >= prefixes[prefix_index].factor * 1000) {
		prefix_index++;
		if (prefix_index > ARRAY_SIZE(prefixes)) {
			// Value is too high to be encoded
			return false;
		}
	}
	int16_t preDot = val / prefixes[prefix_index].factor;
	float postDot = val / prefixes[prefix_index].factor - preDot;
	uint8_t postDotLen = len - 1 	// sign
			- 1 					// decimal point
			- 1 					// prefix
			- 1;					// at least one digit before decimal point
	if (preDot >= 100) {
		*to++ = preDot / 100 + '0';
		postDotLen--;
	}
	if (preDot >= 10) {
		*to++ = (preDot%100) / 10 + '0';
		postDotLen--;
	}
	*to++ = (preDot%10) + '0';
	*to++ = '.';
	while (postDotLen > 0) {
		// remove part before decimal point
		postDot -= (int) postDot;
		postDot *= 10;
		*to++ = (int) postDot + '0';
		postDotLen--;
	}
	*to++ = prefixes[prefix_index].name;
	*to = 0;
	return true;
}

// TODO merge with common_LeastDigitValueFromString?
uint8_t Unit::ValueFromString(int32_t *value, char *s,
		const Unit::unit *unit[]) {
	*value = 0;
	int8_t sign = 1;
	uint32_t dotDivider = 0;
	/* skip any leading spaces */
	while (*s && *s == ' ') {
		s++;
	}
	if(!*s) {
		/* string contains only spaces */
		return 0;
	}
	/* evaluate value part */
	while (*s && (*s == '-' || isdigit((uint8_t )*s) || *s == '.')) {
		if (*s == '-') {
			if (sign == -1) {
				/* this is the second negative */
				return 0;
			}
			sign = -1;
		} else if (*s == '.') {
			if (dotDivider) {
				/* already seen a dot */
				return 0;
			} else {
				dotDivider = 1;
			}
		} else {
			/* must be a digit */
			*value *= 10;
			*value += *s - '0';
			dotDivider *= 10;
		}
		s++;
	}
	if(!dotDivider)
		dotDivider = 1;
	/* finished the value part, now look for matching unit */
	/* skip any trailing spaces */
	while (*s && *s == ' ') {
		s++;
	}
	if(!*s) {
		/* string contains no unit */
		if (unit == Unit::None) {
			*value *= sign;
			return 1;
		}
		return 0;
	}
	/* try to find matching unit */
	while (*unit) {
		if (!strncmp((*unit)->name, s, strlen((*unit)->name))) {
			/* this unit matches the value string */
			*value = (int64_t) (*value) * (*unit)->factor / dotDivider;
			*value *= sign;
			return 1;
		}
		unit++;
	}
	/* no matching unit found */
	return 0;
}

int32_t Unit::ValueFromString(const char* s, uint32_t multiplier) {
	int32_t value = 0;
	int8_t sign = 1;
	uint32_t dotDivider = 0;
	/* skip any leading spaces */
	while (*s && *s == ' ') {
		s++;
	}
	if(!*s) {
		/* string contains only spaces */
		return 0;
	}
	/* evaluate value part */
	while (*s && (*s == '-' || isdigit((uint8_t )*s) || *s == '.')) {
		if (*s == '-') {
			if (sign == -1) {
				/* this is the second negative */
				return 0;
			}
			sign = -1;
		} else if (*s == '.') {
			if (dotDivider) {
				/* already seen a dot */
				return 0;
			} else {
				dotDivider = 1;
			}
		} else {
			/* must be a digit */
			value *= 10;
			value += *s - '0';
			dotDivider *= 10;
		}
		s++;
	}
	if(!dotDivider)
		dotDivider = 1;

	value = (((int64_t) value) * multiplier) / dotDivider;
	value *= sign;
	return value;
}
