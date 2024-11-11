/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#pragma once

#include <chcore/type.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: consider where to put this header file. */

enum AUDIO_REQ {
        AUDIO_PLAY = 1,
        AUDIO_CONNECT,
        AUDIO_DISCONNECT,
};

/* number of samples per frame */
#define FRAME_SIZE 512
/* number of frames per buffer */
#define NUM_OF_FRAMES 6
/* number of samples per buffer */
#define BUFFER_SIZE (FRAME_SIZE * NUM_OF_FRAMES)
struct audio_buffer {
        s64 read_offset;
        s64 write_offset;
        s64 frames_latency_max;
        union {
                /* nBitsPerSample * nChannels == 32 */
                u32 buffer_4_byte[NUM_OF_FRAMES][FRAME_SIZE];
                u32 buffer_4_byte_flat[BUFFER_SIZE];
                /* nBitsPerSample * nChannels == 16 */
                u16 buffer_2_byte[NUM_OF_FRAMES][FRAME_SIZE];
                u16 buffer_2_byte_flat[BUFFER_SIZE];
                /* nBitsPerSample * nChannels == 8 */
                u8 buffer_1_byte[NUM_OF_FRAMES][FRAME_SIZE];
                u8 buffer_1_byte_flat[BUFFER_SIZE];
        };
};

struct audio_msg {
        enum AUDIO_REQ req;
        union {
                struct {
                        uint32_t nBitsPerSample;
                        uint32_t nChannels;
                        uint32_t nSamples;
                } sound_play_req;
                struct {
                        uint32_t nBitsPerSample;
                        uint32_t nChannels;
                        // cap_t buffer on slot 0;
                } conn_req;
                struct {
                        int32_t key;
                } disconn_req;
        };
};

#ifdef __cplusplus
}
#endif
