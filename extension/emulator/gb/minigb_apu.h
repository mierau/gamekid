/**
 * minigb_apu is released under the terms listed within the LICENSE file.
 *
 * minigb_apu emulates the audio processing unit (APU) of the Game Boy. This
 * project is based on MiniGBS by Alex Baines: https://github.com/baines/MiniGBS
 */

#ifndef minigb_apu_h
#define minigb_apu_h

#include <stdint.h>

/**
 * Fill allocated buffer "data" with "len" number of 32-bit floating point
 * samples (native endian order) in stereo interleaved format.
 */
// void audio_callback(void *ptr, uint8_t *data, int len);

/**
 * Read audio register at given address "addr".
 */
uint8_t audio_read(const uint16_t addr);

/**
 * Write "val" to audio register at given address "addr".
 */
void audio_write(const uint16_t addr, const uint8_t val);

/**
 * Initialise audio driver.
 */
void audio_init(void);


extern int GKAudioSourceCallback(void* context, int16_t* left, int16_t* right, int len);

#endif