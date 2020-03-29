/*
 * Sound.hpp
 *
 *  Created on: Mar 29, 2020
 *      Author: jan
 */

#ifndef SOUND_H_
#define SOUND_H_

#ifdef __cplusplus
#include <cstdint>
namespace Sound {

void Beep(uint16_t freq, uint16_t duration);

}

extern "C" {
#endif
void SoundTimerOvf();
#ifdef __cplusplus
}
#endif


#endif /* SOUND_H_ */
