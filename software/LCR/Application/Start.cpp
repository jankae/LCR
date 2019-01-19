#include "Start.h"
#include "log.h"
#include "Algorithm.hpp"
#include <cstring>
#include "LCR.hpp"
#include "Waveform.hpp"
#include <cmath>

void Start() {
	log_init();
	LOG(Log_App, LevelInfo, "Start");

	LCR::Init();
	double real, imag;
	constexpr uint32_t freq = 1000;
	Waveform::Setup(freq, 2048);
	Waveform::Enable();
	while (1) {
		LCR::Measure(1000, &real, &imag);
		LOG(Log_App, LevelInfo, "R=%fOhm", real);
		if(imag < 0) {
			double cap = 1.0 / (2 * M_PI * -imag * freq);
			LOG(Log_App, LevelInfo, "C=%fnF", cap * 1000000000);
		} else if (imag > 0) {
			double ind = imag / (2*M_PI * freq);
			LOG(Log_App, LevelInfo, "L=%fuH", ind * 1000000);
		}
		vTaskDelay(5000);
	}
}
