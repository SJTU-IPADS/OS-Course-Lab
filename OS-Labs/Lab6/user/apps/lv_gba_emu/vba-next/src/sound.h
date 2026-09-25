#ifndef SOUND_H
#define SOUND_H

#include "sound_blargg.h"

#define SGCNT0_H 0x82
#define FIFOA_L 0xa0
#define FIFOA_H 0xa2
#define FIFOB_L 0xa4
#define FIFOB_H 0xa6

#define BLIP_BUFFER_ACCURACY 16
#define BLIP_PHASE_BITS 8
#define BLIP_WIDEST_IMPULSE_ 16
#define BLIP_BUFFER_EXTRA_ 18
#define BLIP_RES 256
#define BLIP_RES_MIN_ONE 255
#define BLIP_SAMPLE_BITS 30
#define BLIP_READER_DEFAULT_BASS 9
#define BLIP_DEFAULT_LENGTH 250		/* 1/4th of a second */

#define BUFS_SIZE 3
#define STEREO 2

#define	CLK_MUL	GB_APU_OVERCLOCK
#define CLK_MUL_MUL_2 8
#define CLK_MUL_MUL_4 16
#define CLK_MUL_MUL_6 24
#define CLK_MUL_MUL_8 32
#define CLK_MUL_MUL_15 60
#define CLK_MUL_MUL_32 128
#define DAC_BIAS 7

#define PERIOD_MASK 0x70
#define SHIFT_MASK 0x07

#define PERIOD2_MASK 0x1FFFF

#define BANK40_MASK 0x40
#define BANK_SIZE 32
#define BANK_SIZE_MIN_ONE 31
#define BANK_SIZE_DIV_TWO 16

/* 11-bit frequency in NRx3 and NRx4*/
#define GB_OSC_FREQUENCY() (((regs[4] & 7) << 8) + regs[3])

#define	WAVE_TYPE	0x100
#define NOISE_TYPE	0x200
#define MIXED_TYPE	WAVE_TYPE | NOISE_TYPE
#define TYPE_INDEX_MASK	0xFF

struct blip_buffer_state_t
{
        uint32_t offset_;
        int32_t reader_accum_;
        int32_t buf [BLIP_BUFFER_EXTRA_];
};


/* Begins reading from buffer. Name should be unique to the current block.*/
#define BLIP_READER_BEGIN( name, blip_buffer ) \
        const int32_t * name##_reader_buf = (blip_buffer).buffer_;\
        int32_t name##_reader_accum = (blip_buffer).reader_accum_

/* Advances to next sample*/
#define BLIP_READER_NEXT( name, bass ) \
        (void) (name##_reader_accum += *name##_reader_buf++ - (name##_reader_accum >> (bass)))

/* Ends reading samples from buffer. The number of samples read must now be removed
   using Blip_Buffer::remove_samples(). */
#define BLIP_READER_END( name, blip_buffer ) \
        (void) ((blip_buffer).reader_accum_ = name##_reader_accum)

#define BLIP_READER_ADJ_( name, offset ) (name##_reader_buf += offset)

#define BLIP_READER_NEXT_IDX_( name, idx ) {\
        name##_reader_accum -= name##_reader_accum >> BLIP_READER_DEFAULT_BASS;\
        name##_reader_accum += name##_reader_buf [(idx)];\
}

#define BLIP_READER_NEXT_RAW_IDX_( name, idx ) {\
        name##_reader_accum -= name##_reader_accum >> BLIP_READER_DEFAULT_BASS; \
        name##_reader_accum += *(int32_t const*) ((char const*) name##_reader_buf + (idx)); \
}

#if defined (_M_IX86) || defined (_M_IA64) || defined (__i486__) || \
                defined (__x86_64__) || defined (__ia64__) || defined (__i386__)
        #define BLIP_CLAMP_( in ) in < -0x8000 || 0x7FFF < in
#else
        #define BLIP_CLAMP_( in ) (int16_t) in != in
#endif

/* Clamp sample to int16_t range */
#define BLIP_CLAMP( sample, out ) { if ( BLIP_CLAMP_( (sample) ) ) (out) = ((sample) >> 24) ^ 0x7FFF; }
#define GB_ENV_DAC_ENABLED() (regs[2] & 0xF8)	/* Non-zero if DAC is enabled*/
#define GB_NOISE_PERIOD2_INDEX()	(regs[3] >> 4)
#define GB_NOISE_PERIOD2(base)		(base << GB_NOISE_PERIOD2_INDEX())
#define GB_NOISE_LFSR_MASK()		((regs[3] & 0x08) ? ~0x4040 : ~0x4000)
#define GB_WAVE_DAC_ENABLED() (regs[0] & 0x80)	/* Non-zero if DAC is enabled*/

#define reload_sweep_timer() \
        sweep_delay = (regs [0] & PERIOD_MASK) >> 4; \
        if ( !sweep_delay ) \
                sweep_delay = 8;



void soundSetVolume( float );

// Manages muting bitmask. The bits control the following channels:
// 0x001 Pulse 1
// 0x002 Pulse 2
// 0x004 Wave
// 0x008 Noise
// 0x100 PCM 1
// 0x200 PCM 2
void soundPause (void);
void soundResume (void);
void soundSetSampleRate(long sampleRate);
void soundReset (void);
void soundEvent_u8( int gb_addr, uint32_t addr, uint8_t  data );
void soundEvent_u8_parallel(int gb_addr[], uint32_t address[], uint8_t data[]);
void soundEvent_u16( uint32_t addr, uint16_t data );
void soundTimerOverflow( int which );
void process_sound_tick_fn (void);
void soundSaveGameMem(uint8_t *& data);
void soundReadGameMem(const uint8_t *& data, int version);

extern int SOUND_CLOCK_TICKS;   // Number of 16.8 MHz clocks between calls to soundTick()
extern int soundTicks;          // Number of 16.8 MHz clocks until soundTick() will be called

#endif // SOUND_H
