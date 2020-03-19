#include <util.h>
#include "stm.h"

#include "time.h"

#include "log.h"

#ifdef HAL_RTC_MODULE_ENABLED
extern RTC_HandleTypeDef hrtc;

int stm_set_rtc(uint32_t timestamp) {
	time_t time = timestamp;
	struct tm *timeinfo = gmtime(&time);
	if (timeinfo) {
		RTC_TimeTypeDef time;
		RTC_DateTypeDef date;
		time.Hours = timeinfo->tm_hour;
		time.Minutes = timeinfo->tm_min;
		time.Seconds = timeinfo->tm_sec;
		time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
		time.SecondFraction = 0;
		time.StoreOperation = RTC_STOREOPERATION_SET;
		time.SubSeconds = 0;
		time.TimeFormat = RTC_FORMAT_BIN;
		date.Year = timeinfo->tm_year - 100; // convert from timeinfo offset (1900) to RTC offset (2000)
		date.Month = timeinfo->tm_mon + 1;
		date.Date = timeinfo->tm_mday;
		date.WeekDay = timeinfo->tm_wday + 1;
		LOG(Log_App, LevelDebug, "RTC set: %d:%d:%d %d/%d/%d", time.Hours,
				time.Minutes, time.Seconds, date.Date, date.Month, date.Year);
		if (HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) {
			return -1;
		}
		if (HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK) {
			return -1;
		}
		return 0;
	} else {
		return -1;
	}
}
uint32_t stm_get_rtc() {
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;
	// read time first to lock shadow registers
	HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	LOG(Log_App, LevelDebug, "RTC get: %d:%d:%d %d/%d/%d", time.Hours,
			time.Minutes, time.Seconds, date.Date, date.Month, date.Year);

	return unixtime((uint16_t) date.Year + 2000, date.Month, date.Date, time.Hours,
			time.Minutes, time.Seconds);
}
#endif
