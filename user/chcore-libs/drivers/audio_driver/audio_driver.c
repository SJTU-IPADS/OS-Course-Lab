#include <chcore/ipc.h>
#include <chcore-internal/audio_defs.h>
#include <chcore/services.h>
#include <chcore/memory.h>
#include "audio_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct audio_buffer_handler {
        int key;
        cap_t pmo;
        struct audio_buffer* buffer;
        int bytes_per_sample;
        int sample_write_offset;
} audio_buffer_handler_t;

#define sample_to_bytes(x, bytes_per_sample) ((x) * (bytes_per_sample))

static ipc_struct_t* ipc_struct;

void play_sound(const void* buffer, uint32_t nBitsPerSample, uint32_t nChannels,
                uint32_t nSamples)
{
        ipc_msg_t* ipc_msg;
        struct audio_msg* msg;
        int ret;
        cap_t pmo = usys_create_pmo(nSamples * nChannels * nBitsPerSample / 8,
                                    PMO_DATA);
        if (pmo < 0) {
                fprintf(stderr, "Fail to create pmo for sound data.\n");
                return;
        }
        void* sound_data = chcore_auto_map_pmo(
                pmo, nSamples * nChannels * nBitsPerSample / 8, VM_WRITE);
        memcpy(sound_data, buffer, nSamples * nChannels * nBitsPerSample / 8);
        ipc_struct = chcore_conn_srv(SERVER_AUDIO);
        ipc_msg = ipc_create_msg_with_cap(
                ipc_struct, sizeof(struct audio_msg), 1);
        msg = (struct audio_msg*)ipc_get_msg_data(ipc_msg);
        msg->req = AUDIO_PLAY;
        msg->sound_play_req.nBitsPerSample = nBitsPerSample;
        msg->sound_play_req.nChannels = nChannels;
        msg->sound_play_req.nSamples = nSamples;
        ipc_set_msg_cap(ipc_msg, 0, pmo);
        ret = ipc_call(ipc_struct, ipc_msg);
        if (ret < 0) {
                fprintf(stderr, "Fail to play sound with error %d.\n", ret);
        }
        chcore_auto_unmap_pmo(
                pmo, sound_data, nSamples * nChannels * nBitsPerSample / 8);
        usys_revoke_cap(pmo);
        ipc_destroy_msg(ipc_msg);
}

struct audio_buffer_handler* connect_audio_driver(uint32_t nBitsPerSample,
                                                  uint32_t nChannels,
                                                  int64_t frames_latency_max)
{
        ipc_msg_t* ipc_msg;
        struct audio_msg* msg;
        struct audio_buffer* buffer;
        cap_t pmo;
        int ret = 0;
        ipc_struct = chcore_conn_srv(SERVER_AUDIO);
        pmo = usys_create_pmo(sizeof(struct audio_buffer), PMO_DATA);
        if (pmo < 0) {
                goto fail;
        }
        buffer = (struct audio_buffer*)chcore_auto_map_pmo(
                pmo, sizeof(struct audio_buffer), VM_READ | VM_WRITE);
        buffer->frames_latency_max = frames_latency_max;
        buffer->write_offset = UINT64_MAX;
        ipc_msg = ipc_create_msg_with_cap(
                ipc_struct, sizeof(struct audio_msg), 1);
        msg = (struct audio_msg*)ipc_get_msg_data(ipc_msg);
        msg->req = AUDIO_CONNECT;
        msg->conn_req.nBitsPerSample = nBitsPerSample;
        msg->conn_req.nChannels = nChannels;
        ipc_set_msg_cap(ipc_msg, 0, pmo);
        ret = ipc_call(ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);
        if (ret < 0) {
                fprintf(stderr,
                        "Fail to connect to audio service with error %d.\n",
                        ret);
                goto fail_revoke_pmo;
        }
        struct audio_buffer_handler* handler =
                (struct audio_buffer_handler*)malloc(
                        sizeof(struct audio_buffer_handler));
        handler->key = ret;
        handler->buffer = buffer;
        handler->pmo = pmo;
        handler->sample_write_offset = 0;
        handler->bytes_per_sample = nBitsPerSample * nChannels / 8;
        if (handler->bytes_per_sample != 1 && handler->bytes_per_sample != 2
            && handler->bytes_per_sample != 4) {
                fprintf(stderr, "Invalid bytes per sample.\n");
                goto fail_free_handler;
        }
        return handler;
fail_free_handler:
        free(handler);
fail_revoke_pmo:
        chcore_auto_unmap_pmo(pmo, buffer, sizeof(struct audio_buffer));
        usys_revoke_cap(pmo);
fail:
        return NULL;
}

void disconnect_audio_driver(struct audio_buffer_handler* handler)
{
        ipc_msg_t* ipc_msg;
        struct audio_msg* msg;
        int ret;
        ipc_struct = chcore_conn_srv(SERVER_AUDIO);
        ipc_msg = ipc_create_msg(ipc_struct, sizeof(struct audio_msg));
        msg = (struct audio_msg*)ipc_get_msg_data(ipc_msg);
        msg->req = AUDIO_DISCONNECT;
        msg->disconn_req.key = handler->key;
        ret = ipc_call(ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);
        if (ret < 0) {
                fprintf(stderr,
                        "Fail to disconnect to audio service with error %d.\n",
                        ret);
        }
        chcore_auto_unmap_pmo(handler->pmo, handler->buffer, sizeof(struct audio_buffer));
        usys_revoke_cap(handler->pmo);
        free(handler);
}

int write_to_audio_buffer(struct audio_buffer_handler* handler,
                          const void* data, size_t nSamples)
{
        int ret;
        u64 start;
        u64 end;
        const uint8_t* data_buffer_8 = data;
        const uint16_t* data_buffer_16 = data;
        const uint32_t* data_buffer_32 = data;

        if (nSamples > BUFFER_SIZE) {
                nSamples = BUFFER_SIZE;
        }
        start = handler->sample_write_offset % (BUFFER_SIZE);
        end = (start + nSamples) % (BUFFER_SIZE);
        switch (handler->bytes_per_sample) {
        case 1:
                if (end <= start) {
                        memcpy(handler->buffer->buffer_1_byte_flat + start,
                               data_buffer_8,
                               sample_to_bytes(BUFFER_SIZE - start, 1));
                        memcpy(handler->buffer->buffer_1_byte_flat,
                               data_buffer_8 + (BUFFER_SIZE - start),
                               sample_to_bytes(end, 1));
                } else {
                        memcpy(handler->buffer->buffer_1_byte_flat + start,
                               data_buffer_8,
                               sample_to_bytes(nSamples, 1));
                }
                break;
        case 2:
                if (end <= start) {
                        memcpy(handler->buffer->buffer_2_byte_flat + start,
                               data_buffer_16,
                               sample_to_bytes(BUFFER_SIZE - start, 2));
                        memcpy(handler->buffer->buffer_2_byte_flat,
                               data_buffer_16 + (BUFFER_SIZE - start),
                               sample_to_bytes(end, 2));
                } else {
                        memcpy(handler->buffer->buffer_2_byte_flat + start,
                               data_buffer_16,
                               sample_to_bytes(nSamples, 2));
                }
                break;
        case 4:
                if (end <= start) {
                        memcpy(handler->buffer->buffer_4_byte_flat + start,
                               data_buffer_32,
                               sample_to_bytes(BUFFER_SIZE - start, 4));
                        memcpy(handler->buffer->buffer_4_byte_flat,
                               data_buffer_32 + (BUFFER_SIZE - start),
                               sample_to_bytes(end, 4));
                } else {
                        memcpy(handler->buffer->buffer_4_byte_flat + start,
                               data_buffer_32,
                               sample_to_bytes(nSamples, 4));
                }
                break;
        default:
                fprintf(stderr, "Invalid bytes per sample.\n");
                return -1;
        }
        handler->sample_write_offset += nSamples;
        handler->buffer->write_offset =
                handler->sample_write_offset / FRAME_SIZE;
        return nSamples;
}