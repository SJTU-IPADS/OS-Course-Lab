/**
 * @file gba_port_audio.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "../gba_emu/gba_emu.h"
#include "port.h"
#include <pthread.h>
#include <unistd.h>

#include <chcore/ipc.h>
#include <chcore/syscall.h>
#include <chcore/services.h>
#include <chcore-internal/procmgr_defs.h>
#include <string.h>
#include <libpipe.h>
#include <audio_driver.h>

/*********************
 *      DEFINES
 *********************/

#define PCM_DEVICE "default"

#define PAGE_SIZE 0x1000

#define FRAMES_LATENCY_MAX 2

#define AUDIO_LOCK()
#define AUDIO_UNLOCK()


/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    int16_t* buffer;
    int head;
    int tail;
    int size;
} audio_fifo_t;

typedef struct {
    int sample_rate;
    audio_fifo_t fifo;
    audio_buffer_handler_t* handler;
    u64 wirte_sample_offset;
} audio_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/


/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static size_t gba_audio_output_cb(void* user_data, const int16_t* data, size_t frames);
static int audio_init(audio_ctx_t* ctx);

int gba_audio_init(lv_obj_t* gba_emu)
{
    int ret;
    static audio_ctx_t ctx;
    int sample_rate = lv_gba_emu_get_audio_sample_rate(gba_emu);
    printf("sample rate: %d\n", sample_rate);
    LV_ASSERT(sample_rate > 0);
    ctx.sample_rate = sample_rate;
    ctx.wirte_sample_offset = 0;
    ret = audio_init(&ctx);
    if (ret < 0) {
        return ret;
    }
    lv_gba_emu_set_audio_output_cb(gba_emu, gba_audio_output_cb, &ctx);
    return 0;
}

void gba_audo_deinit(lv_obj_t* gba_emu)
{
    // TODO：Uncomment the following code when you use raspi4 real board.
    // disconnect_audio_driver(((audio_ctx_t*)lv_gba_emu_get_audio_output_user_data(gba_emu))->handler);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int audio_init(audio_ctx_t* ctx)
{
    // TODO：Uncomment the following code when you use raspi4 real board.
    // ctx->handler = connect_audio_driver(16, 2, FRAMES_LATENCY_MAX);
    // if(ctx->handler == NULL){
    //     return -1;
    // }
    // printf("audio driver connected\n");
    return 0;
}

static size_t gba_audio_output_cb(void* user_data, const int16_t* data, size_t samples)
{
    // TODO：Uncomment the following code when you use raspi4 real board.
    // int ret;
    // audio_ctx_t* ctx;
    // ctx = user_data;
    // return write_to_audio_buffer(ctx->handler, data, samples);
    return samples;
}
