#include "Sound.h"
#include "stm.h"

static uint32_t periodCnt = 0;

extern TIM_HandleTypeDef htim8;

void Sound::Beep(uint16_t freq, uint16_t duration) {
	HAL_TIM_OC_Stop(&htim8, TIM_CHANNEL_2);
	HAL_TIM_Base_Stop_IT(&htim8);
	htim8.Instance->CNT = 0;
	htim8.Instance->ARR = 1000000 / freq;
	htim8.Instance->CCR2 = 100000 / freq;
	periodCnt = freq * duration / 1000;
	HAL_TIM_Base_Start_IT(&htim8);
	HAL_TIM_OC_Start(&htim8, TIM_CHANNEL_2);
}

void SoundTimerOvf() {
	if(!--periodCnt) {
		// beep finished
		htim8.Instance->CCR2 = 0;
	}
}
