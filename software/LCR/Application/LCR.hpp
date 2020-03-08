/*
 * LCR.hpp
 *
 *  Created on: Jan 19, 2019
 *      Author: jan
 */

#ifndef LCR_HPP_
#define LCR_HPP_

#include <cstdint>

namespace LCR {

bool Init();
bool Measure(uint32_t frequency, double *real, double *imag, double *phase);
bool CalibratePhase();

}

#endif /* LCR_HPP_ */
