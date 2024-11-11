#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <bits/errno.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include <circle/actled.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/types.h>
#include <circle/pwmsounddevice.h>
#include <circle/devicenameservice.h>

#include <chcore/defs.h>
#include <chcore/syscall.h>
#include <chcore/bug.h>
#include <chcore/ipc.h>
#include <chcore-internal/audio_defs.h>

#include "../circle.hpp"

#define SOUND_SAMPLES (sizeof Sound / sizeof Sound[0] / SOUND_CHANNELS)

#ifdef PAGE_SIZE
#undef PAGE_SIZE
#endif
#define PAGE_SIZE 0x1000

class PWMSoundDriver : public CPWMSoundDevice {
    public:
        PWMSoundDriver(void);
        ~PWMSoundDriver(void);

        void Play(void *pSoundData, unsigned nSamples, unsigned nChannels,
                  unsigned nBitsPerSample);
        static PWMSoundDriver *Get(void)
        {
                return singleton;
        }

        unsigned GetChunk(u32 *pBuffer, unsigned nChunkSize);

        void SetBuffer(struct audio_buffer *buffer)
        {
                this->buffer = buffer;
        }

        void SetChannelPara(unsigned nChannels, unsigned nBitsPerSample)
        {
                this->m_nChannels = nChannels;
                this->m_nBitsPerSample = nBitsPerSample;
        }

        enum audio_driver_status {
                IDLE,
                PLAY_SEGMENT,
                REALTIME_PLAY,
        };
        void SetStatus(audio_driver_status status)
        {
                this->status = status;
        }
        uint32_t RequestRealtimePlay(void)
        {
                this->status = REALTIME_PLAY;
                this->key = rand();
                return (this->key);
        }
        bool RequestStopRealtimePlay(int32_t key)
        {
                if (key != this->key) {
                        return false;
                }
                this->status = IDLE;
                return true;
        }
        bool IsRealtimePlayStop(void)
        {
                return (this->status != REALTIME_PLAY);
        }
        void RequestSegmentPlay(void)
        {
                this->status = PLAY_SEGMENT;
        }
        bool IsBusy(void)
        {
                return (this->status != IDLE);
        }

    private:
        // do not change this order
        struct audio_buffer *buffer;

        uint32_t *p_SoundBuffer;

        static PWMSoundDriver *singleton;

        audio_driver_status status;

        int32_t key;
};

PWMSoundDriver *PWMSoundDriver::singleton = nullptr;

PWMSoundDriver::PWMSoundDriver(void)
        : CPWMSoundDevice(CInterruptSystem::Get())
{
        p_SoundBuffer = new uint32_t[m_nChunkSize];
        singleton = this;
        status = IDLE;
        srand(time(NULL));
}

PWMSoundDriver::~PWMSoundDriver(void)
{
}

void PWMSoundDriver::Play(void *pSoundData, unsigned nSamples,
                          unsigned nChannels, unsigned nBitsPerSample)
{
        Playback(pSoundData, nSamples, nChannels, nBitsPerSample);
        usleep(2000);
        while (PlaybackActive()) {
        };
        this->status = IDLE;
}

unsigned PWMSoundDriver::GetChunk(u32 *pBuffer, unsigned nChunkSize)
{
        assert(pBuffer != 0);
        assert(nChunkSize > 0);
        assert((nChunkSize & 1) == 0);
        assert(this->status != IDLE);

        if (this->status == PLAY_SEGMENT) {
                return CPWMSoundDevice::GetChunk(pBuffer, nChunkSize);
        }

        unsigned nResult = 0;
        unsigned int bytesPerSample = m_nBitsPerSample * m_nChannels / 8;

        if (unlikely(buffer->read_offset > buffer->write_offset)) {
                return 0;
        }
        if (buffer->write_offset - buffer->read_offset
            > buffer->frames_latency_max) {
                buffer->read_offset =
                        buffer->write_offset - buffer->frames_latency_max;
        }
        switch (bytesPerSample) {
        case 1:
                memcpy(p_SoundBuffer,
                       (void *)buffer->buffer_1_byte[buffer->read_offset
                                                     % NUM_OF_FRAMES],
                       sizeof(buffer->buffer_1_byte[buffer->read_offset
                                                    % NUM_OF_FRAMES]));
                break;
        case 2:
                memcpy(p_SoundBuffer,
                       (void *)buffer->buffer_2_byte[buffer->read_offset
                                                     % NUM_OF_FRAMES],
                       sizeof(buffer->buffer_2_byte[buffer->read_offset
                                                    % NUM_OF_FRAMES]));
                break;
        case 4:
                memcpy(p_SoundBuffer,
                       (void *)buffer->buffer_4_byte[buffer->read_offset
                                                     % NUM_OF_FRAMES],
                       sizeof(buffer->buffer_4_byte[buffer->read_offset
                                                    % NUM_OF_FRAMES]));
                break;
        default:
                assert(0);
        }
        assert(FRAME_SIZE <= nChunkSize / 2);
        m_nSamples = FRAME_SIZE;
        buffer->read_offset++;
        m_pSoundData = (u8 *)p_SoundBuffer;
        if (m_nSamples == 0) {
                return nResult;
        }

        assert(m_pSoundData != 0);
        assert(m_nChannels == 1 || m_nChannels == 2);
        assert(m_nBitsPerSample == 8 || m_nBitsPerSample == 16);

        for (unsigned nSample = 0; nSample < nChunkSize / 2;) // 2 channels on
                                                              // output
        {
                unsigned nValue = *m_pSoundData++;
                if (m_nBitsPerSample > 8) {
                        nValue |= (unsigned)*m_pSoundData++ << 8;
                        nValue = (nValue + 0x8000) & 0xFFFF; // signed ->
                                                             // unsigned (16
                                                             // bit)
                }

                if (m_nBitsPerSample >= m_nRangeBits) {
                        nValue >>= m_nBitsPerSample - m_nRangeBits;
                } else {
                        nValue <<= m_nRangeBits - m_nBitsPerSample;
                }

                unsigned nValue2 = nValue;
                if (m_nChannels == 2) {
                        nValue2 = *m_pSoundData++;
                        if (m_nBitsPerSample > 8) {
                                nValue2 |= (unsigned)*m_pSoundData++ << 8;
                                nValue2 = (nValue2 + 0x8000)
                                          & 0xFFFF; // signed
                                                    // ->
                                                    // unsigned
                                                    // (16
                                                    // bit)
                        }

                        if (m_nBitsPerSample >= m_nRangeBits) {
                                nValue2 >>= m_nBitsPerSample - m_nRangeBits;
                        } else {
                                nValue2 <<= m_nRangeBits - m_nBitsPerSample;
                        }
                }

                if (!m_bChannelsSwapped) {
                        pBuffer[nSample++] = nValue;
                        pBuffer[nSample++] = nValue2;
                } else {
                        pBuffer[nSample++] = nValue2;
                        pBuffer[nSample++] = nValue;
                }

                nResult += 2;

                if (--m_nSamples == 0) {
                        break;
                }
        }
        return nResult;
}

static int PWM_sound_play(audio_msg *msg, cap_t data)
{
        size_t size;
        void *sound;
        int ret = 0;
        if (PWMSoundDriver::Get()->IsBusy()) {
                ret = -EBUSY;
                goto out;
        }
        PWMSoundDriver::Get()->RequestSegmentPlay();
        size = msg->sound_play_req.nSamples * msg->sound_play_req.nChannels
               * msg->sound_play_req.nBitsPerSample / 8;
        sound = chcore_auto_map_pmo(data, size, VM_READ | VM_WRITE);
        if (sound == 0) {
                ret = -ECAPBILITY;
                goto out;
        }
        PWMSoundDriver::Get()->Play(sound,
                                    msg->sound_play_req.nSamples,
                                    msg->sound_play_req.nChannels,
                                    msg->sound_play_req.nBitsPerSample);
        chcore_auto_unmap_pmo(data, (unsigned long)sound, size);
out:
        return ret;
}

struct play_thread_args {
        uint32_t nBitsPerSample;
        uint32_t nChannels;
        struct audio_buffer *buffer;
};

void *play_thread(void *args)
{
        struct play_thread_args *play_args = (struct play_thread_args *)args;
        uint32_t nBitsPerSample = play_args->nBitsPerSample;
        uint32_t nChannels = play_args->nChannels;
        PWMSoundDriver::Get()->SetChannelPara(nChannels, nBitsPerSample);
        PWMSoundDriver::Get()->SetBuffer(play_args->buffer);
        play_args->buffer->read_offset = 0;
        free(args);
        while (!(PWMSoundDriver::Get()->IsRealtimePlayStop())) {
                PWMSoundDriver::Get()->Start();
                while (PWMSoundDriver::Get()->IsActive()) {
                        usys_yield();
                }
        }
        return NULL;
}

static int connect(ipc_msg_t *ipc_msg, uint32_t nBitsPerSample,
                   uint32_t nChannels)
{
        cap_t pmo;
        pthread_t tid;
        struct play_thread_args *args;
        int ret;
        if (PWMSoundDriver::Get()->IsBusy()) {
                ret = -EBUSY;
                goto out;
        }
        pmo = ipc_get_msg_cap(ipc_msg, 0);

        args = (struct play_thread_args *)malloc(
                sizeof(struct play_thread_args));
        args->nBitsPerSample = nBitsPerSample;
        args->nChannels = nChannels;
        args->buffer = (audio_buffer *)chcore_auto_map_pmo(
                pmo, sizeof(struct audio_buffer), VM_READ | VM_WRITE);
        if (args->buffer == NULL) {
                return -ECAPBILITY;
        }
        ret = PWMSoundDriver::Get()->RequestRealtimePlay();
        pthread_create(&tid, NULL, play_thread, (void *)(args));
out:
        return ret;
}

static int disconnect(ipc_msg_t *ipc_msg, int32_t key)
{
        if ((PWMSoundDriver::Get()->RequestStopRealtimePlay(key)) == false){
                return -EPERM;
        } 
        PWMSoundDriver::Get()->Cancel();
        return 0;
}

DEFINE_SERVER_HANDLER(PWM_sound_dispatch)
{
        struct audio_msg *req;
        int ret = 0;
        cap_t cap = ipc_get_msg_cap(ipc_msg, 0);
        req = (struct audio_msg *)ipc_get_msg_data(ipc_msg);
        switch (req->req) {
        case AUDIO_PLAY:
                ret = PWM_sound_play(req, cap);
                ipc_return(ipc_msg, ret);
        case AUDIO_CONNECT:
                ret = connect(ipc_msg,
                              req->conn_req.nBitsPerSample,
                              req->conn_req.nChannels);
                ipc_return(ipc_msg, ret);
        case AUDIO_DISCONNECT:
                ret = disconnect(ipc_msg, req->disconn_req.key);
                ipc_return(ipc_msg, ret);
        default:
                break;
        }
        ipc_return(ipc_msg, -EINVAL);
}

void *audio(void *args)
{
        int *thread_ret = (int *)malloc(sizeof(int));
        *thread_ret = 0;
        bind_to_cpu(3);
#if 0
	void *token;
	/*
	 * A token for starting the procmgr server.
	 * This is just for preventing users run procmgr in the Shell.
	 * It is actually harmless to remove this.
	 */
	token = strstr(argv[0], KERNEL_SERVER);
	if (token == NULL) {
		printf("procmgr: I am a system server instead of an (Shell) "
		      "application. Bye!\n");
		usys_exit(-1);
	}
#endif
        __attribute__((unused)) PWMSoundDriver *driver = new PWMSoundDriver;
        *thread_ret = ipc_register_server(PWM_sound_dispatch,
                                          DEFAULT_CLIENT_REGISTER_HANDLER);

        return thread_ret;
}