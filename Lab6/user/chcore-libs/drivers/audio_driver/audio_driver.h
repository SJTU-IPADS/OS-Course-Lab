#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <stdint.h>
#include <chcore/type.h>

typedef struct audio_buffer_handler audio_buffer_handler_t;

void play_sound(const void* buffer, uint32_t nBitsPerSample, uint32_t nChannels, uint32_t nSamples);

audio_buffer_handler_t* connect_audio_driver(uint32_t nBitsPerSample, uint32_t nChannels, int64_t frames_latency_max);

void disconnect_audio_driver(audio_buffer_handler_t* handler);

int write_to_audio_buffer(audio_buffer_handler_t* handler, const void* data, size_t nSamples);

#endif /* AUDIO_DRIVER_H */