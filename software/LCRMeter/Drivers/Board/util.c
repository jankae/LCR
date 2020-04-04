/*
 * util.c
 *
 *  Created on: 22.11.2018
 *      Author: felix
 */


#include <util.h>


uint32_t unixtime(int year, int month, int day,
              int hour, int minute, int sec)
{
  const short days_since_year[12] =
    {0,31,59,90,120,151,181,212,243,273,304,334};

  int leapyears = ((year-1)-1968)/4
                  - ((year-1)-1900)/100
                  + ((year-1)-1600)/400;

  long days_since_1970 = (year-1970)*365 + leapyears
                      + days_since_year[month-1] + day-1;

  if ( (month>2) && (year%4==0 && (year%100!=0 || year%400==0)) )
	  days_since_1970 += 1; // leapday

  return sec + 60 * ( minute + 60 * (hour + 24*days_since_1970) );
}


int32_t util_Map(int32_t value, int32_t scaleFromLow, int32_t scaleFromHigh,
		int32_t scaleToLow, int32_t scaleToHigh) {
	int32_t result;
	value -= scaleFromLow;
	int32_t rangeFrom = scaleFromHigh - scaleFromLow;
	int32_t rangeTo = scaleToHigh - scaleToLow;
	result = ((int64_t) value * rangeTo) / rangeFrom;
	result += scaleToLow;
	return result;
}
float util_MapF(float value, float scaleFromLow, float scaleFromHigh,
		float scaleToLow, float scaleToHigh) {
	float result;
	value -= scaleFromLow;
	float rangeFrom = scaleFromHigh - scaleFromLow;
	float rangeTo = scaleToHigh - scaleToLow;
	result = value * rangeTo / rangeFrom;
	result += scaleToLow;
	return result;
}

#define CRC32_POLYGON 0xEDB88320

uint32_t util_crc32(uint32_t crc, const void *data, uint32_t len) {
	uint8_t *u8buf = (uint8_t*) data;
	int k;

	crc = ~crc;
	while (len--) {
		crc ^= *u8buf++;
		for (k = 0; k < 8; k++)
			crc = crc & 1 ? (crc >> 1) ^ CRC32_POLYGON : crc >> 1;
	}
	return ~crc;
}
