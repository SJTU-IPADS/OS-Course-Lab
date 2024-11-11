/* Copyright (C) 2003-2007 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "sound.h"

#include "gba.h"
#include "globals.h"

#include "memory.h"
#include "port.h"
#include "system.h"

#define NR10 0x60
#define NR11 0x62
#define NR12 0x63
#define NR13 0x64
#define NR14 0x65
#define NR21 0x68
#define NR22 0x69
#define NR23 0x6c
#define NR24 0x6d
#define NR30 0x70
#define NR31 0x72
#define NR32 0x73
#define NR33 0x74
#define NR34 0x75
#define NR41 0x78
#define NR42 0x79
#define NR43 0x7c
#define NR44 0x7d
#define NR50 0x80
#define NR51 0x81
#define NR52 0x84

/* 1/100th of a second */
#define SOUND_CLOCK_TICKS_ 280896

/*============================================================
	CLASS DECLS
============================================================ */

class Blip_Buffer
{
	public:
		const char * set_sample_rate( long samples_per_sec, int msec_length = 1000 / 4 );
		void clear( void);
		void save_state( blip_buffer_state_t* out );
		void load_state( blip_buffer_state_t const& in );
		uint32_t clock_rate_factor( long clock_rate ) const;
		long clock_rate_;
		int length_;		/* Length of buffer in milliseconds*/
		long sample_rate_;	/* Current output sample rate*/
		uint32_t factor_;
		uint32_t offset_;
		int32_t * buffer_;
		int32_t buffer_size_;
		int32_t reader_accum_;
		Blip_Buffer();
		~Blip_Buffer();
	private:
		Blip_Buffer( const Blip_Buffer& );
		Blip_Buffer& operator = ( const Blip_Buffer& );
};

class Blip_Synth
{
	public:
	Blip_Buffer* buf;
	int delta_factor;

	Blip_Synth();

	void volume( float v ) { delta_factor = int ((v * 1.0) * (1L << BLIP_SAMPLE_BITS) + 0.5); }
	void offset( int32_t, int delta, Blip_Buffer* ) const;
	void offset_resampled( uint32_t, int delta, Blip_Buffer* ) const;
	void offset_inline( int32_t t, int delta, Blip_Buffer* buf ) const {
		offset_resampled( t * buf->factor_ + buf->offset_, delta, buf );
	}
};

class Gb_Osc
{
	public:
	Blip_Buffer* outputs [4];	/* NULL, right, left, center*/
	Blip_Buffer* output;		/* where to output sound*/
	uint8_t * regs;			/* osc's 5 registers*/
	int mode;			/* mode_dmg, mode_cgb, mode_agb*/
	int dac_off_amp;		/* amplitude when DAC is off*/
	int last_amp;			/* current amplitude in Blip_Buffer*/
	Blip_Synth const* good_synth;
	Blip_Synth  const* med_synth;

	int delay;			/* clocks until frequency timer expires*/
	int length_ctr;			/* length counter*/
	unsigned phase;			/* waveform phase (or equivalent)*/
	bool enabled;			/* internal enabled flag*/

	void clock_length();
	void reset();
	protected:
	void update_amp( int32_t, int new_amp );
	int write_trig( int frame_phase, int max_len, int old_data );
};

class Gb_Env : public Gb_Osc
{
	public:
	int  env_delay;
	int  volume;
	bool env_enabled;

	void clock_envelope();
	bool write_register( int frame_phase, int reg, int old_data, int data );

	void reset()
	{
		env_delay = 0;
		volume    = 0;
		Gb_Osc::reset();
	}
	private:
	void zombie_volume( int old, int data );
	int reload_env_timer();
};

class Gb_Square : public Gb_Env
{
	public:
	bool write_register( int frame_phase, int reg, int old_data, int data );
	void run( int32_t, int32_t );

	void reset()
	{
		Gb_Env::reset();
		delay = 0x40000000; /* TODO: something less hacky (never clocked until first trigger)*/
	}
	private:
	/* Frequency timer period*/
	int period() const { return (2048 - GB_OSC_FREQUENCY()) * (CLK_MUL_MUL_4); }
};

class Gb_Sweep_Square : public Gb_Square
{
	public:
	int  sweep_freq;
	int  sweep_delay;
	bool sweep_enabled;
	bool sweep_neg;

	void clock_sweep();
	void write_register( int frame_phase, int reg, int old_data, int data );

	void reset()
	{
		sweep_freq    = 0;
		sweep_delay   = 0;
		sweep_enabled = false;
		sweep_neg     = false;
		Gb_Square::reset();
	}
	private:
	void calc_sweep( bool update );
};

class Gb_Noise : public Gb_Env
{
	public:
	int divider; /* noise has more complex frequency divider setup*/

	void run( int32_t, int32_t );
	void write_register( int frame_phase, int reg, int old_data, int data );

	void reset()
	{
		divider = 0;
		Gb_Env::reset();
		delay = CLK_MUL_MUL_4; /* TODO: remove?*/
	}
};

class Gb_Wave : public Gb_Osc
{
	public:
	int sample_buf;		/* last wave RAM byte read (hardware has this as well)*/
	int agb_mask;		/* 0xFF if AGB features enabled, 0 otherwise*/
	uint8_t* wave_ram;	/* 32 bytes (64 nybbles), stored in APU*/

	void write_register( int frame_phase, int reg, int old_data, int data );
	void run( int32_t, int32_t );

	/* Reads/writes wave RAM*/
	int read( unsigned addr ) const;
	void write( unsigned addr, int data );

	void reset()
	{
		sample_buf = 0;
		Gb_Osc::reset();
	}

	private:
	friend class Gb_Apu;

	/* Frequency timer period*/
	int period() const { return (2048 - GB_OSC_FREQUENCY()) * (CLK_MUL_MUL_2); }

	void corrupt_wave();

	/* Wave index that would be accessed, or -1 if no access would occur*/
	int access( unsigned addr ) const;
};

/*============================================================
	INLINE CLASS FUNCS
============================================================ */

INLINE void Blip_Synth::offset_resampled( uint32_t time, int delta, Blip_Buffer* blip_buf ) const
{
	int32_t left, right, phase;
	int32_t *buf;

	delta *= delta_factor;
	buf = blip_buf->buffer_ + (time >> BLIP_BUFFER_ACCURACY);
	phase = (int) (time >> (BLIP_BUFFER_ACCURACY - BLIP_PHASE_BITS) & BLIP_RES_MIN_ONE);

	left = buf [0] + delta;

	right = (delta >> BLIP_PHASE_BITS) * phase;

	left  -= right;
	right += buf [1];

	buf [0] = left;
	buf [1] = right;
}

INLINE void Blip_Synth::offset( int32_t t, int delta, Blip_Buffer* buf ) const
{
        offset_resampled( t * buf->factor_ + buf->offset_, delta, buf );
}

INLINE int Gb_Wave::read( unsigned addr ) const
{
	int index;

	if(enabled)
		index = access( addr );
	else
		index = addr & 0x0F;
	
	unsigned char const * wave_bank = &wave_ram[(~regs[0] & BANK40_MASK) >> 2 & agb_mask];

	return (index < 0 ? 0xFF : wave_bank[index]);
}

INLINE void Gb_Wave::write( unsigned addr, int data )
{
	int index;

	if(enabled)
		index = access( addr );
	else
		index = addr & 0x0F;
	
	unsigned char * wave_bank = &wave_ram[(~regs[0] & BANK40_MASK) >> 2 & agb_mask];

	if ( index >= 0 )
		wave_bank[index] = data;;
}

static int16_t   soundFinalWave [1600];
long  soundSampleRate    = 22050;
int   SOUND_CLOCK_TICKS  = SOUND_CLOCK_TICKS_;
int   soundTicks         = SOUND_CLOCK_TICKS_;

static int soundEnableFlag   = 0x3ff; /* emulator channels enabled*/
static float const apu_vols [4] = { -0.25f, -0.5f, -1.0f, -0.25f };

static const int table [0x40] =
{
		0xFF10,     0,0xFF11,0xFF12,0xFF13,0xFF14,     0,     0,
		0xFF16,0xFF17,     0,     0,0xFF18,0xFF19,     0,     0,
		0xFF1A,     0,0xFF1B,0xFF1C,0xFF1D,0xFF1E,     0,     0,
		0xFF20,0xFF21,     0,     0,0xFF22,0xFF23,     0,     0,
		0xFF24,0xFF25,     0,     0,0xFF26,     0,     0,     0,
		     0,     0,     0,     0,     0,     0,     0,     0,
		0xFF30,0xFF31,0xFF32,0xFF33,0xFF34,0xFF35,0xFF36,0xFF37,
		0xFF38,0xFF39,0xFF3A,0xFF3B,0xFF3C,0xFF3D,0xFF3E,0xFF3F,
};

typedef struct
{
	int last_amp;
	int last_time;
	int shift;
	Blip_Buffer* output;
} gba_pcm_t;

typedef struct
{
	bool enabled;
	uint8_t   fifo [32];
	int  count;
	int  dac;
	int  readIndex;
	int  writeIndex;
	int     which;
	int  timer;
	gba_pcm_t pcm;
} gba_pcm_fifo_t;

static gba_pcm_fifo_t   pcm [2];


static Blip_Synth pcm_synth; // 32 kHz, 16 kHz, 8 kHz

static Blip_Buffer bufs_buffer [BUFS_SIZE];
static int mixer_samples_read;

static void gba_pcm_init (void)
{
	pcm[0].pcm.output    = 0;
	pcm[0].pcm.last_time = 0;
	pcm[0].pcm.last_amp  = 0;
	pcm[0].pcm.shift     = 0;

	pcm[1].pcm.output    = 0;
	pcm[1].pcm.last_time = 0;
	pcm[1].pcm.last_amp  = 0;
	pcm[1].pcm.shift     = 0;
}

static void gba_pcm_apply_control( int pcm_idx, int idx )
{
	int ch = 0;
	pcm[pcm_idx].pcm.shift = ~ioMem [SGCNT0_H] >> (2 + idx) & 1;

	if ( (ioMem [NR52] & 0x80) )
		ch = ioMem [SGCNT0_H+1] >> (idx << 2) & 3;

	Blip_Buffer* out = 0;
	switch ( ch )
	{
		case 1:
			out = &bufs_buffer[1];
			break;
		case 2:
			out = &bufs_buffer[0];
			break;
		case 3:
			out = &bufs_buffer[2];
			break;
	}

	if ( pcm[pcm_idx].pcm.output != out )
	{
		if ( pcm[pcm_idx].pcm.output )
			pcm_synth.offset( SOUND_CLOCK_TICKS - soundTicks, -pcm[pcm_idx].pcm.last_amp, pcm[pcm_idx].pcm.output );
		pcm[pcm_idx].pcm.last_amp = 0;
		pcm[pcm_idx].pcm.output = out;
	}
}

/*============================================================
	GB APU
============================================================ */

/* 0: Square 1, 1: Square 2, 2: Wave, 3: Noise */
#define OSC_COUNT 4

/* Resets hardware to initial power on state BEFORE boot ROM runs. Mode selects*/
/* sound hardware. Additional AGB wave features are enabled separately.*/
#define MODE_AGB	2

#define START_ADDR	0xFF10
#define END_ADDR	0xFF3F

/* Reads and writes must be within the START_ADDR to END_ADDR range, inclusive.*/
/* Addresses outside this range are not mapped to the sound hardware.*/
#define REGISTER_COUNT	48
#define REGS_SIZE 64

/* Clock rate that sound hardware runs at.
 * formula: 4194304 * 4 
 * */
#define CLOCK_RATE 16777216

struct gb_apu_t
{
	bool		reduce_clicks_;
	uint8_t		regs[REGS_SIZE]; // last values written to registers
	int32_t		last_time;	// time sound emulator has been run to
	int32_t		frame_time;	// time of next frame sequencer action
	int32_t		frame_period;       // clocks between each frame sequencer step
	int32_t         frame_phase;    // phase of next frame sequencer step
	float		volume_;
	Gb_Osc*		oscs [OSC_COUNT];
	Gb_Sweep_Square square1;
	Gb_Square       square2;
	Gb_Wave         wave;
	Gb_Noise        noise;
	Blip_Synth	good_synth;
	Blip_Synth	med_synth;
} gb_apu;

// Format of save state. Should be stable across versions of the library,
// with earlier versions properly opening later save states. Includes some
// room for expansion so the state size shouldn't increase.
struct gb_apu_state_t
{
	// Values stored as plain int so your code can read/write them easily.
	// Structure can NOT be written to disk, since format is not portable.
	typedef int val_t;

	enum { format0 = 0x50414247 };

	val_t format;   // format of all following data
	val_t version;  // later versions just add fields to end

	unsigned char regs [0x40];
	val_t frame_time;
	val_t frame_phase;

	val_t sweep_freq;
	val_t sweep_delay;
	val_t sweep_enabled;
	val_t sweep_neg;
	val_t noise_divider;
	val_t wave_buf;

	val_t delay      [4];
	val_t length_ctr [4];
	val_t phase      [4];
	val_t enabled    [4];

	val_t env_delay   [3];
	val_t env_volume  [3];
	val_t env_enabled [3];

	val_t unused  [13]; // for future expansion
};

#define VOL_REG 0xFF24
#define STEREO_REG 0xFF25
#define STATUS_REG 0xFF26
#define WAVE_RAM 0xFF30
#define POWER_MASK 0x80

#define OSC_COUNT 4

static void gb_apu_reduce_clicks( bool reduce )
{
	gb_apu.reduce_clicks_ = reduce;

	/* Click reduction makes DAC off generate same output as volume 0*/
	int dac_off_amp = 0;

	gb_apu.oscs[0]->dac_off_amp = dac_off_amp;
	gb_apu.oscs[1]->dac_off_amp = dac_off_amp;
	gb_apu.oscs[2]->dac_off_amp = dac_off_amp;
	gb_apu.oscs[3]->dac_off_amp = dac_off_amp;

	/* AGB always eliminates clicks on wave channel using same method*/
	gb_apu.wave.dac_off_amp = -DAC_BIAS;
}

static void gb_apu_synth_volume( int iv )
{
	float v = gb_apu.volume_ * 0.60 / OSC_COUNT / 15 /*steps*/ / 8 /*master vol range*/ * iv;
	gb_apu.good_synth.volume( v );
	gb_apu.med_synth .volume( v );
}

static void gb_apu_apply_volume (void)
{
	int data, left, right, vol_tmp;
	data  = gb_apu.regs [VOL_REG - START_ADDR];
	left  = data >> 4 & 7;
	right = data & 7;
	vol_tmp = left < right ? right : left;
	gb_apu_synth_volume( vol_tmp + 1 );
}

static void gb_apu_silence_osc( Gb_Osc& o )
{
	int delta;

	delta = -o.last_amp;
	if ( delta )
	{
		o.last_amp = 0;
		if ( o.output )
		{
			gb_apu.med_synth.offset( gb_apu.last_time, delta, o.output );
		}
	}
}

static void gb_apu_run_until_( int32_t end_time )
{
	int32_t time;

	do{
		/* run oscillators*/
		time = end_time;
		if ( time > gb_apu.frame_time )
			time = gb_apu.frame_time;

		gb_apu.square1.run( gb_apu.last_time, time );
		gb_apu.square2.run( gb_apu.last_time, time );
		gb_apu.wave   .run( gb_apu.last_time, time );
		gb_apu.noise  .run( gb_apu.last_time, time );
		gb_apu.last_time = time;

		if ( time == end_time )
			break;

		/* run frame sequencer*/
		gb_apu.frame_time += gb_apu.frame_period * CLK_MUL;
		switch ( gb_apu.frame_phase++ )
		{
			case 2:
			case 6:
				/* 128 Hz*/
				gb_apu.square1.clock_sweep();
			case 0:
			case 4:
				/* 256 Hz*/
				gb_apu.square1.clock_length();
				gb_apu.square2.clock_length();
				gb_apu.wave   .clock_length();
				gb_apu.noise  .clock_length();
				break;

			case 7:
				/* 64 Hz*/
				gb_apu.frame_phase = 0;
				gb_apu.square1.clock_envelope();
				gb_apu.square2.clock_envelope();
				gb_apu.noise  .clock_envelope();
		}
	}while(1);
}

INLINE void Gb_Sweep_Square::write_register( int frame_phase, int reg, int old_data, int data )
{
        if ( reg == 0 && sweep_enabled && sweep_neg && !(data & 0x08) )
                enabled = false; // sweep negate disabled after used

        if ( Gb_Square::write_register( frame_phase, reg, old_data, data ) )
        {
                sweep_freq = GB_OSC_FREQUENCY();
                sweep_neg = false;
                reload_sweep_timer();
                sweep_enabled = (regs [0] & (PERIOD_MASK | SHIFT_MASK)) != 0;
                if ( regs [0] & SHIFT_MASK )
                        calc_sweep( false );
        }
}

INLINE void Gb_Wave::write_register( int frame_phase, int reg, int old_data, int data )
{
        switch ( reg )
	{
		case 0:
			if ( !GB_WAVE_DAC_ENABLED() )
				enabled = false;
			break;

		case 1:
			length_ctr = 256 - data;
			break;

		case 4:
			if ( write_trig( frame_phase, 256, old_data ) )
			{
				if ( !GB_WAVE_DAC_ENABLED() )
					enabled = false;
				phase = 0;
				delay    = period() + CLK_MUL_MUL_6;
			}
	}
}

INLINE void Gb_Noise::write_register( int frame_phase, int reg, int old_data, int data )
{
        if ( Gb_Env::write_register( frame_phase, reg, old_data, data ) )
        {
                phase = 0x7FFF;
                delay += CLK_MUL_MUL_8;
        }
}

static void gb_apu_write_osc( int index, int reg, int old_data, int data )
{
        reg -= index * 5;
        switch ( index )
	{
		case 0:
			gb_apu.square1.write_register( gb_apu.frame_phase, reg, old_data, data );
			break;
		case 1:
			gb_apu.square2.write_register( gb_apu.frame_phase, reg, old_data, data );
			break;
		case 2:
			gb_apu.wave.write_register( gb_apu.frame_phase, reg, old_data, data );
			break;
		case 3:
			gb_apu.noise.write_register( gb_apu.frame_phase, reg, old_data, data );
			break;
	}
}

static INLINE int gb_apu_calc_output( int osc )
{
	int bits = gb_apu.regs [STEREO_REG - START_ADDR] >> osc;
	return (bits >> 3 & 2) | (bits & 1);
}

static void gb_apu_write_register( int32_t time, unsigned addr, int data )
{
	int reg = addr - START_ADDR;
	if ( (unsigned) reg >= REGISTER_COUNT )
		return;

	if ( addr < STATUS_REG && !(gb_apu.regs [STATUS_REG - START_ADDR] & POWER_MASK) )
		return;	/* Power is off*/

	if ( time > gb_apu.last_time )
		gb_apu_run_until_( time );

	if ( addr >= WAVE_RAM )
	{
		gb_apu.wave.write( addr, data );
	}
	else
	{
		int old_data = gb_apu.regs [reg];
		gb_apu.regs [reg] = data;

		if ( addr < VOL_REG )
			gb_apu_write_osc( reg / 5, reg, old_data, data );	/* Oscillator*/
		else if ( addr == VOL_REG && data != old_data )
		{
			/* Master volume*/
			for ( int i = OSC_COUNT; --i >= 0; )
				gb_apu_silence_osc( *gb_apu.oscs [i] );

			gb_apu_apply_volume();
		}
		else if ( addr == STEREO_REG )
		{
			/* Stereo panning*/
			for ( int i = OSC_COUNT; --i >= 0; )
			{
				Gb_Osc& o = *gb_apu.oscs [i];
				Blip_Buffer* out = o.outputs [gb_apu_calc_output( i )];
				if ( o.output != out )
				{
					gb_apu_silence_osc( o );
					o.output = out;
				}
			} }
		else if ( addr == STATUS_REG && (data ^ old_data) & POWER_MASK )
		{
			/* Power control*/
			gb_apu.frame_phase = 0;
			for ( int i = OSC_COUNT; --i >= 0; )
				gb_apu_silence_osc( *gb_apu.oscs [i] );

			for ( int i = 0; i < 32; i++ )
				gb_apu.regs [i] = 0;

			gb_apu.square1.reset();
			gb_apu.square2.reset();
			gb_apu.wave   .reset();
			gb_apu.noise  .reset();

			gb_apu_apply_volume();

			gb_apu.square1.length_ctr = 64;
			gb_apu.square2.length_ctr = 64;
			gb_apu.wave   .length_ctr = 256;
			gb_apu.noise  .length_ctr = 64;

			gb_apu.regs [STATUS_REG - START_ADDR] = data;
		}
	}
}

static void gb_apu_reset( uint32_t mode, bool agb_wave )
{
	/* Hardware mode*/
	mode = MODE_AGB; /* using AGB wave features implies AGB hardware*/
	gb_apu.wave.agb_mask = 0xFF;
	gb_apu.oscs [0]->mode = mode;
	gb_apu.oscs [1]->mode = mode;
	gb_apu.oscs [2]->mode = mode;
	gb_apu.oscs [3]->mode = mode;
	gb_apu_reduce_clicks( gb_apu.reduce_clicks_ );

	/* Reset state*/
	gb_apu.frame_time  = 0;
	gb_apu.last_time   = 0;
	gb_apu.frame_phase = 0;

	for ( int i = 0; i < 32; i++ )
		gb_apu.regs [i] = 0;

	gb_apu.square1.reset();
	gb_apu.square2.reset();
	gb_apu.wave   .reset();
	gb_apu.noise  .reset();

	gb_apu_apply_volume();

	gb_apu.square1.length_ctr = 64;
	gb_apu.square2.length_ctr = 64;
	gb_apu.wave   .length_ctr = 256;
	gb_apu.noise  .length_ctr = 64;

	/* Load initial wave RAM*/
	static unsigned char const initial_wave [2] [16] = {
		{0x84,0x40,0x43,0xAA,0x2D,0x78,0x92,0x3C,0x60,0x59,0x59,0xB0,0x34,0xB8,0x2E,0xDA},
		{0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF},
	};
	for ( int b = 2; --b >= 0; )
	{
		/* Init both banks (does nothing if not in AGB mode)*/
		gb_apu_write_register( 0, 0xFF1A, b * 0x40 );
		for ( unsigned i = 0; i < sizeof initial_wave [0]; i++ )
			gb_apu_write_register( 0, i + WAVE_RAM, initial_wave [1] [i] );
	}
}

static void gb_apu_new(void)
{
	int i;

	gb_apu.wave.wave_ram = &gb_apu.regs [WAVE_RAM - START_ADDR];

	gb_apu.oscs [0] = &gb_apu.square1;
	gb_apu.oscs [1] = &gb_apu.square2;
	gb_apu.oscs [2] = &gb_apu.wave;
	gb_apu.oscs [3] = &gb_apu.noise;

	for ( i = OSC_COUNT; --i >= 0; )
	{
		Gb_Osc& o = *gb_apu.oscs [i];
		o.regs        = &gb_apu.regs [i * 5];
		o.output      = 0;
		o.outputs [0] = 0;
		o.outputs [1] = 0;
		o.outputs [2] = 0;
		o.outputs [3] = 0;
		o.good_synth  = &gb_apu.good_synth;
		o.med_synth   = &gb_apu.med_synth;
	}

	gb_apu.reduce_clicks_ = false;
	gb_apu.frame_period = 4194304 / 512; /* 512 Hz*/

	gb_apu.volume_ = 1.0;
	gb_apu_reset(MODE_AGB, false);
}



static void gb_apu_set_output( Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right, int osc )
{
	int i;

	i = osc;
	do
	{
		Gb_Osc& o = *gb_apu.oscs [i];
		o.outputs [1] = right;
		o.outputs [2] = left;
		o.outputs [3] = center;
		o.output = o.outputs [gb_apu_calc_output( i )];
		++i;
	}
	while ( i < osc );
}

static void gb_apu_volume( float v )
{
	if ( gb_apu.volume_ != v )
	{
		gb_apu.volume_ = v;
		gb_apu_apply_volume();
	}
}

static void gb_apu_apply_stereo (void)
{
	int i;

	for ( i = OSC_COUNT; --i >= 0; )
	{
		Gb_Osc& o = *gb_apu.oscs [i];
		Blip_Buffer* out = o.outputs [gb_apu_calc_output( i )];
		if ( o.output != out )
		{
			gb_apu_silence_osc( o );
			o.output = out;
		}
	}
}

#define REFLECT( x, y ) (save ?       (io->y) = (x) :         (x) = (io->y)          )

static INLINE const char* gb_apu_save_load( gb_apu_state_t* io, bool save )
{
	int format, version;

	format = io->format0;

	REFLECT( format, format );
	if ( format != io->format0 )
		return "Unsupported sound save state format";

	version = 0;
	REFLECT( version, version );

	/* Registers and wave RAM*/
	if ( save )
		memcpy( io->regs, gb_apu.regs, sizeof io->regs );
	else
		memcpy( gb_apu.regs, io->regs, sizeof(gb_apu.regs) );

	/* Frame sequencer*/
	REFLECT( gb_apu.frame_time,  frame_time  );
	REFLECT( gb_apu.frame_phase, frame_phase );

	REFLECT( gb_apu.square1.sweep_freq,    sweep_freq );
	REFLECT( gb_apu.square1.sweep_delay,   sweep_delay );
	REFLECT( gb_apu.square1.sweep_enabled, sweep_enabled );
	REFLECT( gb_apu.square1.sweep_neg,     sweep_neg );

	REFLECT( gb_apu.noise.divider,         noise_divider );
	REFLECT( gb_apu.wave.sample_buf,       wave_buf );

	return 0;
}

/* second function to avoid inline limits of some compilers*/
static INLINE void gb_apu_save_load2( gb_apu_state_t* io, bool save )
{
	int i;
	for ( i = OSC_COUNT; --i >= 0; )
	{
		Gb_Osc& osc = *gb_apu.oscs [i];
		REFLECT( osc.delay,      delay      [i] );
		REFLECT( osc.length_ctr, length_ctr [i] );
		REFLECT( osc.phase,      phase      [i] );
		REFLECT( osc.enabled,    enabled    [i] );

		if ( i != 2 )
		{
			int j = 2 < i ? 2 : i;
			Gb_Env& env = STATIC_CAST(Gb_Env&,osc);
			REFLECT( env.env_delay,   env_delay   [j] );
			REFLECT( env.volume,      env_volume  [j] );
			REFLECT( env.env_enabled, env_enabled [j] );
		}
	}
}

static void gb_apu_save_state( gb_apu_state_t* out )
{
	(void) gb_apu_save_load( out, true );
	gb_apu_save_load2( out, true );
}

static const char * gb_apu_load_state( gb_apu_state_t const& in )
{
	RETURN_ERR( gb_apu_save_load( CONST_CAST(gb_apu_state_t*,&in), false));
	gb_apu_save_load2( CONST_CAST(gb_apu_state_t*,&in), false );

	gb_apu_apply_stereo();
	gb_apu_synth_volume( 0 );          /* suppress output for the moment*/
	gb_apu_run_until_( gb_apu.last_time );    /* get last_amp updated*/
	gb_apu_apply_volume();             /* now use correct volume*/

	return 0;
}

/*============================================================
	GB OSCS
============================================================ */

#define TRIGGER_MASK 0x80
#define LENGTH_ENABLED 0x40

#define VOLUME_SHIFT_PLUS_FOUR	6
#define SIZE20_MASK 0x20

void Gb_Osc::reset()
{
        output   = 0;
        last_amp = 0;
        delay    = 0;
        phase    = 0;
        enabled  = false;
}

INLINE void Gb_Osc::update_amp( int32_t time, int new_amp )
{
	int delta = new_amp - last_amp;
        if ( delta )
        {
                last_amp = new_amp;
                med_synth->offset( time, delta, output );
        }
}

void Gb_Osc::clock_length()
{
        if ( (regs [4] & LENGTH_ENABLED) && length_ctr )
        {
                if ( --length_ctr <= 0 )
                        enabled = false;
        }
}

INLINE int Gb_Env::reload_env_timer()
{
        int raw = regs [2] & 7;
        env_delay = (raw ? raw : 8);
        return raw;
}

void Gb_Env::clock_envelope()
{
        if ( env_enabled && --env_delay <= 0 && reload_env_timer() )
        {
                int v = volume + (regs [2] & 0x08 ? +1 : -1);
                if ( 0 <= v && v <= 15 )
                        volume = v;
                else
                        env_enabled = false;
        }
}

#define reload_sweep_timer() \
        sweep_delay = (regs [0] & PERIOD_MASK) >> 4; \
        if ( !sweep_delay ) \
                sweep_delay = 8;

void Gb_Sweep_Square::calc_sweep( bool update )
{
	int shift, delta, freq;

        shift = regs [0] & SHIFT_MASK;
        delta = sweep_freq >> shift;
        sweep_neg = (regs [0] & 0x08) != 0;
        freq = sweep_freq + (sweep_neg ? -delta : delta);

        if ( freq > 0x7FF )
                enabled = false;
        else if ( shift && update )
        {
                sweep_freq = freq;

                regs [3] = freq & 0xFF;
                regs [4] = (regs [4] & ~0x07) | (freq >> 8 & 0x07);
        }
}

void Gb_Sweep_Square::clock_sweep()
{
        if ( --sweep_delay <= 0 )
        {
                reload_sweep_timer();
                if ( sweep_enabled && (regs [0] & PERIOD_MASK) )
                {
                        calc_sweep( true  );
                        calc_sweep( false );
                }
        }
}

int Gb_Wave::access( unsigned addr ) const
{
	return addr & 0x0F;
}

// write_register

int Gb_Osc::write_trig( int frame_phase, int max_len, int old_data )
{
        int data = regs [4];

        if ( (frame_phase & 1) && !(old_data & LENGTH_ENABLED) && length_ctr )
        {
                if ( (data & LENGTH_ENABLED))
                        length_ctr--;
        }

        if ( data & TRIGGER_MASK )
        {
                enabled = true;
                if ( !length_ctr )
                {
                        length_ctr = max_len;
                        if ( (frame_phase & 1) && (data & LENGTH_ENABLED) )
                                length_ctr--;
                }
        }

        if ( !length_ctr )
                enabled = false;

        return data & TRIGGER_MASK;
}

INLINE void Gb_Env::zombie_volume( int old, int data )
{
	int v = volume;

	// CGB-05 behavior, very close to AGB behavior as well
	if ( (old ^ data) & 8 )
	{
		if ( !(old & 8) )
		{
			v++;
			if ( old & 7 )
				v++;
		}

		v = 16 - v;
	}
	else if ( (old & 0x0F) == 8 )
		v++;
	volume = v & 0x0F;
}

bool Gb_Env::write_register( int frame_phase, int reg, int old, int data )
{
        int const max_len = 64;

        switch ( reg )
	{
		case 1:
			length_ctr = max_len - (data & (max_len - 1));
			break;

		case 2:
			if ( !GB_ENV_DAC_ENABLED() )
				enabled = false;

			zombie_volume( old, data );

			if ( (data & 7) && env_delay == 8 )
			{
				env_delay = 1;
				clock_envelope(); // TODO: really happens at next length clock
			}
			break;

		case 4:
			if ( write_trig( frame_phase, max_len, old ) )
			{
				volume = regs [2] >> 4;
				reload_env_timer();
				env_enabled = true;
				if ( frame_phase == 7 )
					env_delay++;
				if ( !GB_ENV_DAC_ENABLED() )
					enabled = false;
				return true;
			}
	}
        return false;
}

bool Gb_Square::write_register( int frame_phase, int reg, int old_data, int data )
{
        bool result = Gb_Env::write_register( frame_phase, reg, old_data, data );
        if ( result )
                delay = (delay & (CLK_MUL_MUL_4 - 1)) + period();
        return result;
}


void Gb_Wave::corrupt_wave()
{
        int pos = ((phase + 1) & BANK_SIZE_MIN_ONE) >> 1;
        if ( pos < 4 )
                wave_ram [0] = wave_ram [pos];
        else
                for ( int i = 4; --i >= 0; )
                        wave_ram [i] = wave_ram [(pos & ~3) + i];
}

/* Synthesis*/

void Gb_Square::run( int32_t time, int32_t end_time )
{
        /* Calc duty and phase*/
        static unsigned char const duty_offsets [4] = { 1, 1, 3, 7 };
        static unsigned char const duties       [4] = { 1, 2, 4, 6 };
        int const duty_code = regs [1] >> 6;
        int32_t duty_offset = duty_offsets [duty_code];
        int32_t duty = duties [duty_code];
	/* AGB uses inverted duty*/
	duty_offset -= duty;
	duty = 8 - duty;
        int ph = (phase + duty_offset) & 7;

        /* Determine what will be generated*/
        int vol = 0;
        Blip_Buffer* const out = output;
        if ( out )
        {
                int amp = dac_off_amp;
                if ( GB_ENV_DAC_ENABLED() )
                {
                        if ( enabled )
                                vol = volume;

			amp = -(vol >> 1);

                        /* Play inaudible frequencies as constant amplitude*/
                        if ( GB_OSC_FREQUENCY() >= 0x7FA && delay < CLK_MUL_MUL_32 )
                        {
                                amp += (vol * duty) >> 3;
                                vol = 0;
                        }

                        if ( ph < duty )
                        {
                                amp += vol;
                                vol = -vol;
                        }
                }
                update_amp( time, amp );
        }

        /* Generate wave*/
        time += delay;
        if ( time < end_time )
        {
                int const per = period();
                if ( !vol )
                {
                        /* Maintain phase when not playing*/
                        int count = (end_time - time + per - 1) / per;
                        ph += count; /* will be masked below*/
                        time += (int32_t) count * per;
                }
                else
                {
                        /* Output amplitude transitions*/
                        int delta = vol;
                        do
                        {
                                ph = (ph + 1) & 7;
                                if ( ph == 0 || ph == duty )
                                {
                                        good_synth->offset_inline( time, delta, out );
                                        delta = -delta;
                                }
                                time += per;
                        }
                        while ( time < end_time );

                        if ( delta != vol )
                                last_amp -= delta;
                }
                phase = (ph - duty_offset) & 7;
        }
        delay = time - end_time;
}

/* Quickly runs LFSR for a large number of clocks. For use when noise is generating*/
/* no sound.*/
static unsigned run_lfsr( unsigned s, unsigned mask, int count )
{
	/* optimization used in several places:*/
	/* ((s & (1 << b)) << n) ^ ((s & (1 << b)) << (n + 1)) = (s & (1 << b)) * (3 << n)*/

	if ( mask == 0x4000 )
	{
		if ( count >= 32767 )
			count %= 32767;

		/* Convert from Fibonacci to Galois configuration,*/
		/* shifted left 1 bit*/
		s ^= (s & 1) * 0x8000;

		/* Each iteration is equivalent to clocking LFSR 255 times*/
		while ( (count -= 255) > 0 )
			s ^= ((s & 0xE) << 12) ^ ((s & 0xE) << 11) ^ (s >> 3);
		count += 255;

		/* Each iteration is equivalent to clocking LFSR 15 times*/
		/* (interesting similarity to single clocking below)*/
		while ( (count -= 15) > 0 )
			s ^= ((s & 2) * (3 << 13)) ^ (s >> 1);
		count += 15;

		/* Remaining singles*/
		do{
			--count;
			s = ((s & 2) * (3 << 13)) ^ (s >> 1);
		}while(count >= 0);

		/* Convert back to Fibonacci configuration*/
		s &= 0x7FFF;
	}
	else if ( count < 8)
	{
		/* won't fully replace upper 8 bits, so have to do the unoptimized way*/
		do{
			--count;
			s = (s >> 1 | mask) ^ (mask & -((s - 1) & 2));
		}while(count >= 0);
	}
	else
	{
		if ( count > 127 )
		{
			count %= 127;
			if ( !count )
				count = 127; /* must run at least once*/
		}

		/* Need to keep one extra bit of history*/
		s = s << 1 & 0xFF;

		/* Convert from Fibonacci to Galois configuration,*/
		/* shifted left 2 bits*/
		s ^= (s & 2) << 7;

		/* Each iteration is equivalent to clocking LFSR 7 times*/
		/* (interesting similarity to single clocking below)*/
		while ( (count -= 7) > 0 )
			s ^= ((s & 4) * (3 << 5)) ^ (s >> 1);
		count += 7;

		/* Remaining singles*/
		while ( --count >= 0 )
			s = ((s & 4) * (3 << 5)) ^ (s >> 1);

		/* Convert back to Fibonacci configuration and*/
		/* repeat last 8 bits above significant 7*/
		s = (s << 7 & 0x7F80) | (s >> 1 & 0x7F);
	}

	return s;
}

void Gb_Noise::run( int32_t time, int32_t end_time )
{
        /* Determine what will be generated*/
        int vol = 0;
        Blip_Buffer* const out = output;
        if ( out )
        {
                int amp = dac_off_amp;
                if ( GB_ENV_DAC_ENABLED() )
                {
                        if ( enabled )
                                vol = volume;

			amp = -(vol >> 1);

                        if ( !(phase & 1) )
                        {
                                amp += vol;
                                vol = -vol;
                        }
                }

                /* AGB negates final output*/
		vol = -vol;
		amp    = -amp;

                update_amp( time, amp );
        }

        /* Run timer and calculate time of next LFSR clock*/
        static unsigned char const period1s [8] = { 1, 2, 4, 6, 8, 10, 12, 14 };
        int const period1 = period1s [regs [3] & 7] * CLK_MUL;
        {
                int extra = (end_time - time) - delay;
                int const per2 = GB_NOISE_PERIOD2(8);
                time += delay + ((divider ^ (per2 >> 1)) & (per2 - 1)) * period1;

                int count = (extra < 0 ? 0 : (extra + period1 - 1) / period1);
                divider = (divider - count) & PERIOD2_MASK;
                delay = count * period1 - extra;
        }

        /* Generate wave*/
        if ( time < end_time )
        {
                unsigned const mask = GB_NOISE_LFSR_MASK();
                unsigned bits = phase;

                int per = GB_NOISE_PERIOD2( period1 * 8 );
                if ( GB_NOISE_PERIOD2_INDEX() >= 0xE )
                {
                        time = end_time;
                }
                else if ( !vol )
                {
                        /* Maintain phase when not playing*/
                        int count = (end_time - time + per - 1) / per;
                        time += (int32_t) count * per;
                        bits = run_lfsr( bits, ~mask, count );
                }
                else
                {
                        /* Output amplitude transitions*/
                        int delta = -vol;
                        do
                        {
                                unsigned changed = bits + 1;
                                bits = bits >> 1 & mask;
                                if ( changed & 2 )
                                {
                                        bits |= ~mask;
                                        delta = -delta;
                                        med_synth->offset_inline( time, delta, out );
                                }
                                time += per;
                        }
                        while ( time < end_time );

                        if ( delta == vol )
                                last_amp += delta;
                }
                phase = bits;
        }
}

void Gb_Wave::run( int32_t time, int32_t end_time )
{
        /* Calc volume*/
        static unsigned char const volumes [8] = { 0, 4, 2, 1, 3, 3, 3, 3 };
        int const volume_idx = regs [2] >> 5 & (agb_mask | 3); /* 2 bits on DMG/CGB, 3 on AGB*/
        int const volume_mul = volumes [volume_idx];

        /* Determine what will be generated*/
        int playing = false;
        Blip_Buffer* const out = output;
        if ( out )
        {
                int amp = dac_off_amp;
                if ( GB_WAVE_DAC_ENABLED() )
                {
                        /* Play inaudible frequencies as constant amplitude*/
                        amp = 128; /* really depends on average of all samples in wave*/

                        /* if delay is larger, constant amplitude won't start yet*/
                        if ( GB_OSC_FREQUENCY() <= 0x7FB || delay > CLK_MUL_MUL_15 )
                        {
                                if ( volume_mul )
                                        playing = (int) enabled;

                                amp = (sample_buf << (phase << 2 & 4) & 0xF0) * playing;
                        }

                        amp = ((amp * volume_mul) >> VOLUME_SHIFT_PLUS_FOUR) - DAC_BIAS;
                }
                update_amp( time, amp );
        }

        /* Generate wave*/
        time += delay;
        if ( time < end_time )
        {
                unsigned char const* wave = wave_ram;

                /* wave size and bank*/
                int const flags = regs [0] & agb_mask;
                int const wave_mask = (flags & SIZE20_MASK) | 0x1F;
                int swap_banks = 0;
                if ( flags & BANK40_MASK)
                {
                        swap_banks = flags & SIZE20_MASK;
                        wave += BANK_SIZE_DIV_TWO - (swap_banks >> 1);
                }

                int ph = phase ^ swap_banks;
                ph = (ph + 1) & wave_mask; /* pre-advance*/

                int const per = period();
                if ( !playing )
                {
                        /* Maintain phase when not playing*/
                        int count = (end_time - time + per - 1) / per;
                        ph += count; /* will be masked below*/
                        time += (int32_t) count * per;
                }
                else
                {
                        /* Output amplitude transitions*/
                        int lamp = last_amp + DAC_BIAS;
                        do
                        {
                                /* Extract nybble*/
                                int nybble = wave [ph >> 1] << (ph << 2 & 4) & 0xF0;
                                ph = (ph + 1) & wave_mask;

                                /* Scale by volume*/
                                int amp = (nybble * volume_mul) >> VOLUME_SHIFT_PLUS_FOUR;

                                int delta = amp - lamp;
                                if ( delta )
                                {
                                        lamp = amp;
                                        med_synth->offset_inline( time, delta, out );
                                }
                                time += per;
                        }
                        while ( time < end_time );
                        last_amp = lamp - DAC_BIAS;
                }
                ph = (ph - 1) & wave_mask; /* undo pre-advance and mask position*/

                /* Keep track of last byte read*/
                if ( enabled )
                        sample_buf = wave [ph >> 1];

                phase = ph ^ swap_banks; /* undo swapped banks*/
        }
        delay = time - end_time;
}

/*============================================================
	BLIP BUFFER
============================================================ */

/* Blip_Buffer 0.4.1. http://www.slack.net/~ant */

#define FIXED_SHIFT 12
#define SAL_FIXED_SHIFT 4096
#define TO_FIXED( f )   int ((f) * SAL_FIXED_SHIFT)
#define FROM_FIXED( f ) ((f) >> FIXED_SHIFT)

Blip_Buffer::Blip_Buffer()
{
   factor_       = INT_MAX;
   buffer_       = 0;
   buffer_size_  = 0;
   sample_rate_  = 0;
   clock_rate_   = 0;
   length_       = 0;

   clear();
}

Blip_Buffer::~Blip_Buffer()
{
   if (buffer_)
      free(buffer_);
}

void Blip_Buffer::clear( void)
{
   offset_       = 0;
   reader_accum_ = 0;
   if (buffer_)
      memset( buffer_, 0, (buffer_size_ + BLIP_BUFFER_EXTRA_) * sizeof (int32_t) );
}

const char * Blip_Buffer::set_sample_rate( long new_rate, int msec )
{
   /* start with maximum length that resampled time can represent*/
   long new_size = (ULONG_MAX >> BLIP_BUFFER_ACCURACY) - BLIP_BUFFER_EXTRA_ - 64;
   if ( msec != 0)
   {
      long s = (new_rate * (msec + 1) + 999) / 1000;
      if ( s < new_size )
         new_size = s;
   }

   if ( buffer_size_ != new_size )
   {
      void* p = realloc( buffer_, (new_size + BLIP_BUFFER_EXTRA_) * sizeof *buffer_ );
      if ( !p )
         return "Out of memory";
      buffer_ = (int32_t *) p;
   }

   buffer_size_ = new_size;

   /* update things based on the sample rate*/
   sample_rate_ = new_rate;
   length_ = new_size * 1000 / new_rate - 1;

   /* update these since they depend on sample rate*/
   if ( clock_rate_ )
      factor_ = clock_rate_factor( clock_rate_);

   clear();

   return 0;
}

/* Sets number of source time units per second */

uint32_t Blip_Buffer::clock_rate_factor( long rate ) const
{
   float ratio = (float) sample_rate_ / rate;
   int32_t factor = (int32_t) floor( ratio * (1L << BLIP_BUFFER_ACCURACY) + 0.5 );
   return (uint32_t) factor;
}


/*============================================================
	BLIP SYNTH
============================================================ */

Blip_Synth::Blip_Synth()
{
        buf          = 0;
        delta_factor = 0;
}

void Blip_Buffer::save_state( blip_buffer_state_t* out )
{
        out->offset_       = offset_;
        out->reader_accum_ = reader_accum_;
        memcpy( out->buf, &buffer_ [offset_ >> BLIP_BUFFER_ACCURACY], sizeof out->buf );
}

void Blip_Buffer::load_state( blip_buffer_state_t const& in )
{
        clear();

        offset_       = in.offset_;
        reader_accum_ = in.reader_accum_;
        memcpy( buffer_, in.buf, sizeof(in.buf));
}

/*============================================================
	STEREO BUFFER
============================================================ */

/* Uses three buffers (one for center) and outputs stereo sample pairs. */

#define STEREO_BUFFER_SAMPLES_AVAILABLE() ((long)(bufs_buffer[0].offset_ -  mixer_samples_read) << 1)
#define stereo_buffer_samples_avail() ((((bufs_buffer [0].offset_ >> BLIP_BUFFER_ACCURACY) - mixer_samples_read) << 1))


static const char * stereo_buffer_set_sample_rate( long rate, int msec )
{
        mixer_samples_read = 0;
        for ( int i = BUFS_SIZE; --i >= 0; )
                RETURN_ERR( bufs_buffer [i].set_sample_rate( rate, msec ) );
        return 0; 
}

static void stereo_buffer_clock_rate( long rate )
{
	bufs_buffer[2].factor_ = bufs_buffer [2].clock_rate_factor( rate );
	bufs_buffer[1].factor_ = bufs_buffer [1].clock_rate_factor( rate );
	bufs_buffer[0].factor_ = bufs_buffer [0].clock_rate_factor( rate );
}

static void stereo_buffer_clear (void)
{
        mixer_samples_read = 0;
	bufs_buffer [2].clear();
	bufs_buffer [1].clear();
	bufs_buffer [0].clear();
}

/* mixers use a single index value to improve performance on register-challenged processors
 * offset goes from negative to zero*/

static INLINE void stereo_buffer_mixer_read_pairs( int16_t* out, int count )
{
	/* TODO: if caller never marks buffers as modified, uses mono*/
	/* except that buffer isn't cleared, so caller can encounter*/
	/* subtle problems and not realize the cause.*/
	mixer_samples_read += count;
	int16_t* outtemp = out + count * STEREO;

	/* do left + center and right + center separately to reduce register load*/
	Blip_Buffer* buf = &bufs_buffer [2];
	{
		--buf;
		--outtemp;

		BLIP_READER_BEGIN( side,   *buf );
		BLIP_READER_BEGIN( center, bufs_buffer[2] );

		BLIP_READER_ADJ_( side,   mixer_samples_read );
		BLIP_READER_ADJ_( center, mixer_samples_read );

		int offset = -count;
		do
		{
			int s = (center_reader_accum + side_reader_accum) >> 14;
			BLIP_READER_NEXT_IDX_( side,   offset );
			BLIP_READER_NEXT_IDX_( center, offset );
			BLIP_CLAMP( s, s );

			++offset; /* before write since out is decremented to slightly before end*/
			outtemp [offset * STEREO] = (int16_t) s;
		}while ( offset );

		BLIP_READER_END( side,   *buf );
	}
	{
		--buf;
		--outtemp;

		BLIP_READER_BEGIN( side,   *buf );
		BLIP_READER_BEGIN( center, bufs_buffer[2] );

		BLIP_READER_ADJ_( side,   mixer_samples_read );
		BLIP_READER_ADJ_( center, mixer_samples_read );

		int offset = -count;
		do
		{
			int s = (center_reader_accum + side_reader_accum) >> 14;
			BLIP_READER_NEXT_IDX_( side,   offset );
			BLIP_READER_NEXT_IDX_( center, offset );
			BLIP_CLAMP( s, s );

			++offset; /* before write since out is decremented to slightly before end*/
			outtemp [offset * STEREO] = (int16_t) s;
		}while ( offset );

		BLIP_READER_END( side,   *buf );

		/* only end center once*/
		BLIP_READER_END( center, bufs_buffer[2] );
	}
}

static void blip_buffer_remove_all_samples( long count )
{
	uint32_t new_offset = (uint32_t)count << BLIP_BUFFER_ACCURACY;
	/* BLIP BUFFER #1 */
	bufs_buffer[0].offset_ -= new_offset;
	bufs_buffer[1].offset_ -= new_offset;
	bufs_buffer[2].offset_ -= new_offset;

	/* copy remaining samples to beginning and clear old samples*/
	long remain = (bufs_buffer[0].offset_ >> BLIP_BUFFER_ACCURACY) + BLIP_BUFFER_EXTRA_;
	memmove( bufs_buffer[0].buffer_, bufs_buffer[0].buffer_ + count, remain * sizeof *bufs_buffer[0].buffer_ );
	memset( bufs_buffer[0].buffer_ + remain, 0, count * sizeof(*bufs_buffer[0].buffer_));

	remain = (bufs_buffer[1].offset_ >> BLIP_BUFFER_ACCURACY) + BLIP_BUFFER_EXTRA_;
	memmove( bufs_buffer[1].buffer_, bufs_buffer[1].buffer_ + count, remain * sizeof *bufs_buffer[1].buffer_ );
	memset( bufs_buffer[1].buffer_ + remain, 0, count * sizeof(*bufs_buffer[1].buffer_));

	remain = (bufs_buffer[2].offset_ >> BLIP_BUFFER_ACCURACY) + BLIP_BUFFER_EXTRA_;
	memmove( bufs_buffer[2].buffer_, bufs_buffer[2].buffer_ + count, remain * sizeof *bufs_buffer[2].buffer_ );
	memset( bufs_buffer[2].buffer_ + remain, 0, count * sizeof(*bufs_buffer[2].buffer_));
}

static long stereo_buffer_read_samples( int16_t * out, long out_size )
{
	int pair_count;

        out_size = (STEREO_BUFFER_SAMPLES_AVAILABLE() < out_size) ? STEREO_BUFFER_SAMPLES_AVAILABLE() : out_size;

        pair_count = int (out_size >> 1);
        if ( pair_count )
	{
		stereo_buffer_mixer_read_pairs( out, pair_count );
		blip_buffer_remove_all_samples( mixer_samples_read );
		mixer_samples_read = 0;
	}
        return out_size;
}

static void gba_to_gb_sound_parallel( int * addr, int * addr2 )
{
	uint32_t addr1_table = *addr - 0x60;
	uint32_t addr2_table = *addr2 - 0x60;
	*addr = table [addr1_table];
	*addr2 = table [addr2_table];
}

static void pcm_fifo_write_control( int data, int data2)
{
	pcm[0].enabled = (data & 0x0300) ? true : false;
	pcm[0].timer   = (data & 0x0400) ? 1 : 0;

	if ( data & 0x0800 )
	{
		// Reset
		pcm[0].writeIndex = 0;
		pcm[0].readIndex  = 0;
		pcm[0].count      = 0;
		pcm[0].dac        = 0;
		memset(pcm[0].fifo, 0, sizeof(pcm[0].fifo));
	}

	gba_pcm_apply_control( 0, pcm[0].which );

	if(pcm[0].pcm.output)
	{
		int time = SOUND_CLOCK_TICKS -  soundTicks;

		pcm[0].dac = (int8_t)pcm[0].dac >> pcm[0].pcm.shift;
		int delta = pcm[0].dac - pcm[0].pcm.last_amp;
		if ( delta )
		{
			pcm[0].pcm.last_amp = pcm[0].dac;
			pcm_synth.offset( time, delta, pcm[0].pcm.output );
		}
		pcm[0].pcm.last_time = time;
	}

	pcm[1].enabled = (data2 & 0x0300) ? true : false;
	pcm[1].timer   = (data2 & 0x0400) ? 1 : 0;

	if ( data2 & 0x0800 )
	{
		// Reset
		pcm[1].writeIndex = 0;
		pcm[1].readIndex  = 0;
		pcm[1].count      = 0;
		pcm[1].dac        = 0;
		memset( pcm[1].fifo, 0, sizeof(pcm[1].fifo));
	}

	gba_pcm_apply_control( 1, pcm[1].which );

	if(pcm[1].pcm.output)
	{
		int time = SOUND_CLOCK_TICKS -  soundTicks;

		pcm[1].dac = (int8_t)pcm[1].dac >> pcm[1].pcm.shift;
		int delta = pcm[1].dac - pcm[1].pcm.last_amp;
		if ( delta )
		{
			pcm[1].pcm.last_amp = pcm[1].dac;
			pcm_synth.offset( time, delta, pcm[1].pcm.output );
		}
		pcm[1].pcm.last_time = time;
	}
}

static void soundEvent_u16_parallel(uint32_t address[])
{
	for(int i = 0; i < 8; i++)
	{
		switch ( address[i] )
		{
			case SGCNT0_H:
				//Begin of Write SGCNT0_H
				WRITE16LE( &ioMem [SGCNT0_H], 0 & 0x770F );
				pcm_fifo_write_control(0, 0);

				gb_apu_volume( apu_vols [ioMem [SGCNT0_H] & 3] );
				//End of SGCNT0_H
				break;

			case FIFOA_L:
			case FIFOA_H:
				pcm[0].fifo [pcm[0].writeIndex  ] = 0;
				pcm[0].fifo [pcm[0].writeIndex+1] = 0;
				pcm[0].count += 2;
				pcm[0].writeIndex = (pcm[0].writeIndex + 2) & 31;
				WRITE16LE( &ioMem[address[i]], 0 );
				break;

			case FIFOB_L:
			case FIFOB_H:
				pcm[1].fifo [pcm[1].writeIndex  ] = 0;
				pcm[1].fifo [pcm[1].writeIndex+1] = 0;
				pcm[1].count += 2;
				pcm[1].writeIndex = (pcm[1].writeIndex + 2) & 31;
				WRITE16LE( &ioMem[address[i]], 0 );
				break;

			case 0x88:
				WRITE16LE( &ioMem[address[i]], 0 );
				break;

			default:
				{
					int gb_addr[2]	= {static_cast<int>(address[i] & ~1), static_cast<int>(address[i] | 1)};
					uint32_t address_array[2] = {address[i] & ~ 1, address[i] | 1};
					uint8_t data_array[2] = {0};
					gba_to_gb_sound_parallel(&gb_addr[0], &gb_addr[1]);
					soundEvent_u8_parallel(gb_addr, address_array, data_array);
					break;
				}
		}
	}
}

static void gba_pcm_fifo_timer_overflowed( unsigned pcm_idx )
{
	if ( pcm[pcm_idx].count <= 16 )
	{
		// Need to fill FIFO
		CPUCheckDMA( 3, pcm[pcm_idx].which ? 4 : 2 );

		if ( pcm[pcm_idx].count <= 16 )
		{
			// Not filled by DMA, so fill with 16 bytes of silence
			int reg = pcm[pcm_idx].which ? FIFOB_L : FIFOA_L;

			uint32_t address_array[8] = {static_cast<uint32_t>(reg), static_cast<uint32_t>(reg+2), static_cast<uint32_t>(reg), static_cast<uint32_t>(reg+2), static_cast<uint32_t>(reg), static_cast<uint32_t>(reg+2), static_cast<uint32_t>(reg), static_cast<uint32_t>(reg+2)};
			soundEvent_u16_parallel(address_array);
		}
	}

	// Read next sample from FIFO
	pcm[pcm_idx].count--;
	pcm[pcm_idx].dac = pcm[pcm_idx].fifo [pcm[pcm_idx].readIndex];
	pcm[pcm_idx].readIndex = (pcm[pcm_idx].readIndex + 1) & 31;

	if(pcm[pcm_idx].pcm.output)
	{
		int time = SOUND_CLOCK_TICKS -  soundTicks;

		pcm[pcm_idx].dac = (int8_t)pcm[pcm_idx].dac >> pcm[pcm_idx].pcm.shift;
		int delta = pcm[pcm_idx].dac - pcm[pcm_idx].pcm.last_amp;
		if ( delta )
		{
			pcm[pcm_idx].pcm.last_amp = pcm[pcm_idx].dac;
			pcm_synth.offset( time, delta, pcm[pcm_idx].pcm.output );
		}
		pcm[pcm_idx].pcm.last_time = time;
	}
}

void soundEvent_u8_parallel(int gb_addr[], uint32_t address[], uint8_t data[])
{
	for(uint32_t i = 0; i < 2; i++)
	{
		ioMem[address[i]] = data[i];
		gb_apu_write_register( SOUND_CLOCK_TICKS -  soundTicks, gb_addr[i], data[i] );

		if ( address[i] == NR52 )
		{
			gba_pcm_apply_control(0, 0 );
			gba_pcm_apply_control(1, 1 );
		}
		// TODO: what about byte writes to SGCNT0_H etc.?
	}
}

void soundEvent_u8(int gb_addr, uint32_t address, uint8_t data)
{
	ioMem[address] = data;
	gb_apu_write_register( SOUND_CLOCK_TICKS -  soundTicks, gb_addr, data );

	if ( address == NR52 )
	{
		gba_pcm_apply_control(0, 0 );
		gba_pcm_apply_control(1, 1 );
	}
	// TODO: what about byte writes to SGCNT0_H etc.?
}


void soundEvent_u16(uint32_t address, uint16_t data)
{
	switch ( address )
	{
		case SGCNT0_H:
			//Begin of Write SGCNT0_H
			WRITE16LE( &ioMem [SGCNT0_H], data & 0x770F );
			pcm_fifo_write_control( data, data >> 4);

			gb_apu_volume( apu_vols [ioMem [SGCNT0_H] & 3] );
			//End of SGCNT0_H
			break;

		case FIFOA_L:
		case FIFOA_H:
			pcm[0].fifo [pcm[0].writeIndex  ] = data & 0xFF;
			pcm[0].fifo [pcm[0].writeIndex+1] = data >> 8;
			pcm[0].count += 2;
			pcm[0].writeIndex = (pcm[0].writeIndex + 2) & 31;
			WRITE16LE( &ioMem[address], data );
			break;

		case FIFOB_L:
		case FIFOB_H:
			pcm[1].fifo [pcm[1].writeIndex  ] = data & 0xFF;
			pcm[1].fifo [pcm[1].writeIndex+1] = data >> 8;
			pcm[1].count += 2;
			pcm[1].writeIndex = (pcm[1].writeIndex + 2) & 31;
			WRITE16LE( &ioMem[address], data );
			break;

		case 0x88:
			data &= 0xC3FF;
			WRITE16LE( &ioMem[address], data );
			break;

		default:
			{
				int gb_addr[2]	= {static_cast<int>(address & ~1), (int)(address | 1)};
				uint32_t address_array[2] = {address & ~ 1, static_cast<uint32_t>(address | 1)};
				uint8_t data_array[2] = {(uint8_t)data, (uint8_t)(data >> 8)};
				gba_to_gb_sound_parallel(&gb_addr[0], &gb_addr[1]);
				soundEvent_u8_parallel(gb_addr, address_array, data_array);
				break;
			}
	}
}

void soundTimerOverflow(int timer)
{
	if ( timer == pcm[0].timer && pcm[0].enabled )
		gba_pcm_fifo_timer_overflowed(0);
	if ( timer == pcm[1].timer && pcm[1].enabled )
		gba_pcm_fifo_timer_overflowed(1);
}

void process_sound_tick_fn (void)
{
	// Run sound hardware to present
	pcm[0].pcm.last_time -= SOUND_CLOCK_TICKS;
	if ( pcm[0].pcm.last_time < -2048 )
		pcm[0].pcm.last_time = -2048;

	pcm[1].pcm.last_time -= SOUND_CLOCK_TICKS;
	if ( pcm[1].pcm.last_time < -2048 )
		pcm[1].pcm.last_time = -2048;

	/* Emulates sound hardware up to a specified time, ends current time
	frame, then starts a new frame at time 0 */

	if(SOUND_CLOCK_TICKS > gb_apu.last_time)
		gb_apu_run_until_( SOUND_CLOCK_TICKS );

	gb_apu.frame_time -= SOUND_CLOCK_TICKS;
	gb_apu.last_time -= SOUND_CLOCK_TICKS;

	bufs_buffer[2].offset_ += SOUND_CLOCK_TICKS * bufs_buffer[2].factor_;
	bufs_buffer[1].offset_ += SOUND_CLOCK_TICKS * bufs_buffer[1].factor_;
	bufs_buffer[0].offset_ += SOUND_CLOCK_TICKS * bufs_buffer[0].factor_;


	// dump all the samples available
	// VBA will only ever store 1 frame worth of samples
	int numSamples = stereo_buffer_read_samples( (int16_t*) soundFinalWave, stereo_buffer_samples_avail());
	systemOnWriteDataToSoundBuffer(soundFinalWave, numSamples);
}

static void apply_muting (void)
{
	// PCM
	gba_pcm_apply_control(1, 0 );
	gba_pcm_apply_control(1, 1 );

	// APU
	gb_apu_set_output( &bufs_buffer[2], &bufs_buffer[0], &bufs_buffer[1], 0 );
	gb_apu_set_output( &bufs_buffer[2], &bufs_buffer[0], &bufs_buffer[1], 1 );
	gb_apu_set_output( &bufs_buffer[2], &bufs_buffer[0], &bufs_buffer[1], 2 );
	gb_apu_set_output( &bufs_buffer[2], &bufs_buffer[0], &bufs_buffer[1], 3 );
}


static void remake_stereo_buffer (void)
{
	if ( !ioMem )
		return;

	// Clears pointers kept to old stereo_buffer
	gba_pcm_init();

	// Stereo_Buffer

        mixer_samples_read = 0;
	stereo_buffer_set_sample_rate( soundSampleRate, BLIP_DEFAULT_LENGTH );
	stereo_buffer_clock_rate( CLOCK_RATE );

	// PCM
	pcm [0].which = 0;
	pcm [1].which = 1;

	// APU
	gb_apu_new();
	gb_apu_reset( MODE_AGB, true );

	stereo_buffer_clear();

	soundTicks = SOUND_CLOCK_TICKS;

	apply_muting();

	gb_apu_volume(apu_vols [ioMem [SGCNT0_H] & 3] );

	pcm_synth.volume( 0.66 / 256 * -1);
}

void soundReset (void)
{
	remake_stereo_buffer();
	//Begin of Reset APU
	gb_apu_reset( MODE_AGB, true );

	stereo_buffer_clear();

	soundTicks = SOUND_CLOCK_TICKS;
	//End of Reset APU

	SOUND_CLOCK_TICKS = SOUND_CLOCK_TICKS_;
	soundTicks        = SOUND_CLOCK_TICKS_;

	// Sound Event (NR52)
	int gb_addr = table[NR52 - 0x60];
	if ( gb_addr )
	{
		ioMem[NR52] = 0x80;
		gb_apu_write_register( SOUND_CLOCK_TICKS -  soundTicks, gb_addr, 0x80 );

		gba_pcm_apply_control(0, 0 );
		gba_pcm_apply_control(1, 1 );
	}

	// TODO: what about byte writes to SGCNT0_H etc.?
	// End of Sound Event (NR52)
}

void soundSetSampleRate(long sampleRate)
{
	if ( soundSampleRate != sampleRate )
	{
		soundSampleRate      = sampleRate;
		remake_stereo_buffer();
	}
}

static int dummy_state [16];

#define SKIP( type, name ) { dummy_state, sizeof (type) }

#define LOAD( type, name ) { &name, sizeof (type) }

static struct {
	gb_apu_state_t apu;

	// old state
	int soundDSBValue;
	uint8_t soundDSAValue;
} state;

// New state format
static variable_desc gba_state [] =
{
	// PCM
	LOAD( int, pcm [0].readIndex ),
	LOAD( int, pcm [0].count ),
	LOAD( int, pcm [0].writeIndex ),
	LOAD(uint8_t[32],pcm[0].fifo ),
	LOAD( int, pcm [0].dac ),

	SKIP( int [4], room_for_expansion ),

	LOAD( int, pcm [1].readIndex ),
	LOAD( int, pcm [1].count ),
	LOAD( int, pcm [1].writeIndex ),
	LOAD(uint8_t[32],pcm[1].fifo ),
	LOAD( int, pcm [1].dac ),

	SKIP( int [4], room_for_expansion ),

	// APU
	LOAD( uint8_t [0x40], state.apu.regs ),      // last values written to registers and wave RAM (both banks)
	LOAD( int, state.apu.frame_time ),      // clocks until next frame sequencer action
	LOAD( int, state.apu.frame_phase ),     // next step frame sequencer will run

	LOAD( int, state.apu.sweep_freq ),      // sweep's internal frequency register
	LOAD( int, state.apu.sweep_delay ),     // clocks until next sweep action
	LOAD( int, state.apu.sweep_enabled ),
	LOAD( int, state.apu.sweep_neg ),       // obscure internal flag
	LOAD( int, state.apu.noise_divider ),
	LOAD( int, state.apu.wave_buf ),        // last read byte of wave RAM

	LOAD( int [4], state.apu.delay ),       // clocks until next channel action
	LOAD( int [4], state.apu.length_ctr ),
	LOAD( int [4], state.apu.phase ),       // square/wave phase, noise LFSR
	LOAD( int [4], state.apu.enabled ),     // internal enabled flag

	LOAD( int [3], state.apu.env_delay ),   // clocks until next envelope action
	LOAD( int [3], state.apu.env_volume ),
	LOAD( int [3], state.apu.env_enabled ),

	SKIP( int [13], room_for_expansion ),

	// Emulator
	LOAD( int, soundEnableFlag ),
	LOAD( int, soundTicks ),

	SKIP( int [14], room_for_expansion ),

	{ NULL, 0 }
};

void soundSaveGameMem(uint8_t *& data)
{
	gb_apu_save_state(&state.apu);
	memset(dummy_state, 0, sizeof dummy_state);
	utilWriteDataMem(data, gba_state);
}

void soundReadGameMem(const uint8_t *& in_data, int)
{
	// Prepare APU and default state

	//Begin of Reset APU
	gb_apu_reset( MODE_AGB, true );

	stereo_buffer_clear();

	soundTicks = SOUND_CLOCK_TICKS;
	//End of Reset APU

	gb_apu_save_state( &state.apu );

	utilReadDataMem( in_data, gba_state );

	gb_apu_load_state( state.apu );
	//Begin of Write SGCNT0_H
	int data = (READ16LE( &ioMem [SGCNT0_H] ) & 0x770F);
	WRITE16LE( &ioMem [SGCNT0_H], data & 0x770F );
	pcm_fifo_write_control( data, data >> 4 );

	gb_apu_volume(apu_vols [ioMem [SGCNT0_H] & 3] );
	//End of SGCNT0_H
}

