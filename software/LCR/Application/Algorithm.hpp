/*
 * Algorithm.hpp
 *
 *  Created on: Jan 19, 2019
 *      Author: jan
 */

#ifndef ALGORITHM_HPP_
#define ALGORITHM_HPP_

#include <cstdint>

namespace Algorithm {

using Complex = struct complex {
	double real;
	double imag;
};

Complex Goertzel(int16_t *data, uint16_t N, uint32_t sampleFreq,
		uint32_t targetFreq);

void RemoveDC(int16_t *data, uint16_t n);
double RMS(int16_t *data, uint16_t n);
double PhaseDiff(int16_t *data1, int16_t *data2, uint16_t n);

}

#endif /* ALGORITHM_HPP_ */
