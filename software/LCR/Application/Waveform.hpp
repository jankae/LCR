/*
 * SineGenerator.hpp
 *
 *  Created on: Jan 19, 2019
 *      Author: jan
 */

#ifndef WAVEFORM_HPP_
#define WAVEFORM_HPP_

#include <cstdint>

namespace Waveform {

constexpr double Reference = 3.0;
constexpr uint16_t ADCMax = 4096;

void Setup(uint32_t freq, uint16_t peakval);
void Enable();
void Disable();
void PrintADC();

using Data = struct data {
	int16_t *values;
	uint16_t nvalues;
	uint32_t samplerate;
	uint32_t tone;
};

Data Get();

}

#endif /* WAVEFORM_HPP_ */
