/*
** Haaf's Game Engine 1.8
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** Core functions implementation: audio routines
*/


#include "hge_impl.h"


#include <bass.h>
#pragma comment(lib,"bass.lib")

HEFFECT HGE_CALL HGE_Impl::Effect_Load(const char* filename, 
                                       const uint32_t size) {
    uint32_t size_1;
    BASS_CHANNELINFO info;
    void* data;

    if (!hBass) return 0;
    if (is_silent_) {
        return 1;
    }

    if (size) {
        data = const_cast<void *>(static_cast<const void *>(filename));
        size_1 = size;
    }
    else {
        data = Resource_Load(filename, &size_1);
        if (!data) {
            return NULL;
        }
    }

    auto hs = BASS_SampleLoad(TRUE, data, 0, size_1, 4, BASS_SAMPLE_OVER_VOL);
    if (!hs) {
        const auto hstrm = BASS_StreamCreateFile(TRUE, data, 0, size_1, 
                                                 BASS_STREAM_DECODE);
        if (hstrm) {
            const auto length = static_cast<uint32_t>(BASS_ChannelGetLength(hstrm, BASS_POS_BYTE));
            BASS_ChannelGetInfo(hstrm, &info);
            auto samples = length;
            if (info.chans < 2) {
                samples >>= 1;
            }
            if ((info.flags & BASS_SAMPLE_8BITS) == 0) {
                samples >>= 1;
            }
           hs = BASS_SampleCreate(samples, info.freq, 2, 4,
                                                  info.flags | BASS_SAMPLE_OVER_VOL);
            if (!hs) {
                BASS_StreamFree(hstrm);
                post_error("Can't create sound effect: Not enough memory");
            }
            else {
                byte* buffer = new byte[length];
                BASS_ChannelGetData(hstrm, buffer, length);
				BASS_SampleSetData(hs, buffer);
				delete[] buffer;
                BASS_StreamFree(hstrm);
                if (!hs) {
                    post_error("Can't create sound effect");
                }
            }
        }
    }

    if (!size) {
        Resource_Free(data);
    }
    return hs;
}

HCHANNEL HGE_CALL HGE_Impl::Effect_Play(const HEFFECT eff) {
    if (!hBass) return 0;
    const auto chn = BASS_SampleGetChannel(eff, FALSE);
    BASS_ChannelPlay(chn, TRUE);
    return chn;
}
BOOL BASS_ChannelSetAttributes(DWORD handle, DWORD freq, DWORD volume, DWORD pan)
{
	BOOL ret = BASS_ChannelSetAttribute(handle, BASS_ATTRIB_FREQ, freq);
	ret &= BASS_ChannelSetAttribute(handle, BASS_ATTRIB_VOL, volume / 100.0);
	ret &= BASS_ChannelSetAttribute(handle, BASS_ATTRIB_PAN, pan/100.0);
	return ret;
}
HCHANNEL HGE_CALL HGE_Impl::
Effect_PlayEx(const HEFFECT eff, const int volume, const int pan,
              const float pitch, const bool loop) {
    if (!hBass) return 0;
    BASS_SAMPLE info;
    BASS_SampleGetInfo(eff, &info);

    const auto chn = BASS_SampleGetChannel(eff, FALSE);
    BASS_ChannelSetAttributes(chn, static_cast<int>(pitch * info.freq), volume, pan);

    info.flags &= ~BASS_SAMPLE_LOOP;
    if (loop) {
        info.flags |= BASS_SAMPLE_LOOP;
    }
    BASS_ChannelFlags(chn, info.flags, BASS_SAMPLE_LOOP);
    BASS_ChannelPlay(chn, TRUE);
    return chn;
}


void HGE_CALL HGE_Impl::Effect_Free(const HEFFECT eff) {
    if (!hBass) return;
    BASS_SampleFree(eff);
}


HMUSIC HGE_CALL HGE_Impl::Music_Load(const char* filename, const uint32_t size) {
    void* data;
    uint32_t size_1;

    if (!hBass) return 0;
    if (size) {
        data = const_cast<void *>(static_cast<const void *>(filename));
        //size_1 = size;
    }
    else {
        data = Resource_Load(filename, &size_1);
        if (!data) {
            return 0;
        }
    }

    const auto hm = BASS_MusicLoad(
        TRUE, data, 0, 0, BASS_MUSIC_PRESCAN | BASS_MUSIC_POSRESETEX | BASS_MUSIC_RAMP,
        0);
    if (!hm) {
        post_error("Can't load music");
    }
    if (!size) {
        Resource_Free(data);
    }
    return hm;
}

HCHANNEL HGE_CALL HGE_Impl::Music_Play(const HMUSIC mus, const bool loop,
                                       const int volume, int order, int row) {
    if (!hBass) return 0;
    auto pos = BASS_ChannelGetPosition(mus, BASS_POS_MUSIC_ORDER);
    if (order == -1) {
        order = LOWORD(pos);
    }
    if (row == -1) {
        row = HIWORD(pos);
    }
    BASS_ChannelSetPosition(mus, MAKELONG(order, row), BASS_POS_MUSIC_ORDER);

    BASS_CHANNELINFO info;
    BASS_ChannelGetInfo(mus, &info);
    BASS_ChannelSetAttributes(mus, info.freq, volume, 0);

    info.flags &= ~BASS_SAMPLE_LOOP;
    if (loop) {
        info.flags |= BASS_SAMPLE_LOOP;
    }
    BASS_ChannelFlags(mus, info.flags, BASS_SAMPLE_LOOP);

    BASS_ChannelPlay(mus, FALSE);

    return mus;
}

void HGE_CALL HGE_Impl::Music_Free(const HMUSIC mus) {
    if (!hBass) return;
    BASS_MusicFree(mus);
}

void HGE_CALL HGE_Impl::Music_SetAmplification(const HMUSIC music, const int ampl) {
    if (!hBass) return;
    BASS_ChannelSetAttribute(music, BASS_ATTRIB_MUSIC_AMPLIFY, ampl);
}

int HGE_CALL HGE_Impl::Music_GetAmplification(const HMUSIC music) {
    float value = -1;
	if (hBass) {
		BASS_ChannelGetAttribute(music, BASS_ATTRIB_MUSIC_AMPLIFY,&value);
	}
	return value;
}

int HGE_CALL HGE_Impl::Music_GetLength(const HMUSIC music) {
    if (!hBass) return -1;
    return BASS_ChannelGetLength(music, BASS_POS_MUSIC_ORDER);
}

void HGE_CALL HGE_Impl::Music_SetPos(const HMUSIC music, int order, int row) {
    if (!hBass) return;
    BASS_ChannelSetPosition(music, MAKELONG(order, row), BASS_POS_MUSIC_ORDER);
}

bool HGE_CALL HGE_Impl::Music_GetPos(const HMUSIC music, int* order, int* row) {
    if (!hBass) return false;
    uint32_t pos;
    pos = BASS_ChannelGetPosition(music, BASS_POS_MUSIC_ORDER);
    if (pos == -1) {
        return false;
    }
    *order = LOWORD(pos);
    *row = HIWORD(pos);
    return true;
}

void HGE_CALL HGE_Impl::Music_SetInstrVolume(const HMUSIC music, 
                                             const int instr, const int volume) {
    if (!hBass) return;
    BASS_ChannelSetAttribute(music, BASS_ATTRIB_MUSIC_VOL_INST + instr, volume / 100.0);
}

int HGE_CALL HGE_Impl::Music_GetInstrVolume(const HMUSIC music, const int instr) {
    if (!hBass) return -1;
    float value = -1;
    BASS_ChannelGetAttribute(music, BASS_ATTRIB_MUSIC_VOL_INST + instr,&value);
	return value * 100;
}

void HGE_CALL HGE_Impl::Music_SetChannelVolume(const HMUSIC music, 
                                               const int channel, const int volume) {
    if (!hBass) return;
    BASS_ChannelSetAttribute(music, BASS_ATTRIB_MUSIC_VOL_CHAN + channel, volume/100.0);
}

int HGE_CALL HGE_Impl::Music_GetChannelVolume(const HMUSIC music, const int channel) {
    if (!hBass) return -1;
    float value = -1;
	BASS_ChannelGetAttribute(music, BASS_ATTRIB_MUSIC_VOL_CHAN + channel, &value);
	return value*100;
}

HSTREAM HGE_CALL HGE_Impl::Stream_Load(const char* filename, const uint32_t size) {
    void* data;
    uint32_t _size;

    if (!hBass) return 0;
    if (is_silent_) {
        return 1;
    }

    if (size) {
        data = const_cast<void *>(static_cast<const void *>(filename));
        _size = size;
    }
    else {
        data = Resource_Load(filename, &_size);
        if (!data) {
            return 0;
        }
    }
    const auto hs = BASS_StreamCreateFile(TRUE, data, 0, _size, 0);
    if (!hs) {
        post_error("Can't load stream");
        if (!size) {
            Resource_Free(data);
        }
        return 0;
    }
    if (!size) {
        const auto stmItem = new CStreamList;
        stmItem->hstream = hs;
        stmItem->data = data;
        stmItem->next = sound_streams_;
        sound_streams_ = stmItem;
    }
    return hs;
}

void HGE_CALL HGE_Impl::Stream_Free(const HSTREAM stream) {
    auto stm_item = sound_streams_;
    CStreamList *stm_prev = nullptr;

    if (!hBass) return;
    while (stm_item) {
        if (stm_item->hstream == stream) {
            if (stm_prev) {
                stm_prev->next = stm_item->next;
            }
            else {
                sound_streams_ = stm_item->next;
            }
            Resource_Free(stm_item->data);
            delete stm_item;
            break;
        }
        stm_prev = stm_item;
        stm_item = stm_item->next;
    }
    BASS_StreamFree(stream);
}

HCHANNEL HGE_CALL HGE_Impl::Stream_Play(const HSTREAM stream, const bool loop,
                                        const int volume) {
    if (!hBass) return 0;
    BASS_CHANNELINFO info;
    BASS_ChannelGetInfo(stream, &info);
    BASS_ChannelSetAttributes(stream, info.freq, volume, 0);

    info.flags &= ~BASS_SAMPLE_LOOP;
    if (loop) {
        info.flags |= BASS_SAMPLE_LOOP;
    }
    BASS_ChannelFlags(stream, info.flags, BASS_SAMPLE_LOOP);
    BASS_ChannelPlay(stream, TRUE);
    return stream;
}

void HGE_CALL HGE_Impl::Channel_SetPanning(const HCHANNEL chn, const int pan) {
    if (!hBass) return;
    BASS_ChannelSetAttributes(chn, -1, -1, pan);
}

void HGE_CALL HGE_Impl::Channel_SetVolume(const HCHANNEL chn, const int volume) {
    if (!hBass) return;
    BASS_ChannelSetAttributes(chn, -1, volume, -101);
}

void HGE_CALL HGE_Impl::Channel_SetPitch(const HCHANNEL chn, const float pitch) {
    if (!hBass) return;
    BASS_CHANNELINFO info;
    BASS_ChannelGetInfo(chn, &info);
    BASS_ChannelSetAttributes(chn, static_cast<int>(pitch * info.freq), -1, -101);
}

void HGE_CALL HGE_Impl::Channel_Pause(const HCHANNEL chn) {
    if (!hBass) return;
    BASS_ChannelPause(chn);
}

void HGE_CALL HGE_Impl::Channel_Resume(const HCHANNEL chn) {
    if (!hBass) return;
    BASS_ChannelPlay(chn, FALSE);
}

void HGE_CALL HGE_Impl::Channel_Stop(const HCHANNEL chn) {
    if (!hBass) return;
    BASS_ChannelStop(chn);
}

void HGE_CALL HGE_Impl::Channel_PauseAll() {
    if (!hBass) return;
    BASS_Pause();
}

void HGE_CALL HGE_Impl::Channel_ResumeAll() {
    if (!hBass) return;
    BASS_Start();
}

void HGE_CALL HGE_Impl::Channel_StopAll() {
    if (!hBass) return;
    BASS_Stop();
    BASS_Start();
}

bool HGE_CALL HGE_Impl::Channel_IsPlaying(const HCHANNEL chn) {
    if (!hBass) return false;
    if (BASS_ChannelIsActive(chn) == BASS_ACTIVE_PLAYING) {
        return true;
    }
    return false;
}

float HGE_CALL HGE_Impl::Channel_GetLength(const HCHANNEL chn) {
    if (!hBass) return -1;
    return BASS_ChannelBytes2Seconds(chn, BASS_ChannelGetLength(chn, BASS_POS_BYTE));
}

float HGE_CALL HGE_Impl::Channel_GetPos(const HCHANNEL chn) {
    if (!hBass) return -1;
    return BASS_ChannelBytes2Seconds(chn, BASS_ChannelGetPosition(chn, BASS_POS_BYTE));
}

void HGE_CALL HGE_Impl::Channel_SetPos(const HCHANNEL chn, const float seconds) {
    if (!hBass) return;
    BASS_ChannelSetPosition(chn, BASS_ChannelSeconds2Bytes(chn, seconds), BASS_POS_BYTE);
}

void HGE_CALL HGE_Impl::Channel_SlideTo(const HCHANNEL channel, const float time,
                                        const int volume, const int pan,
                                        const float pitch) {
    if (!hBass) return;
    BASS_CHANNELINFO info;
    BASS_ChannelGetInfo(channel, &info);

    int freq;
    if (pitch == -1) {
        freq = -1;
    }
    else {
        freq = static_cast<int>(pitch * info.freq);
    }

    auto tm = DWORD(time * 1000);
	BASS_ChannelSlideAttribute(channel, BASS_ATTRIB_FREQ, freq, tm);
	BASS_ChannelSlideAttribute(channel, BASS_ATTRIB_VOL, volume, tm);
	BASS_ChannelSlideAttribute(channel, BASS_ATTRIB_PAN, pan/100.0, tm);
}

bool HGE_CALL HGE_Impl::Channel_IsSliding(const HCHANNEL channel) {
return hBass &&
		(BASS_ChannelIsSliding(channel, BASS_ATTRIB_FREQ) || BASS_ChannelIsSliding(channel, BASS_ATTRIB_VOL) || BASS_ChannelIsSliding(channel, BASS_ATTRIB_PAN));
}


//////// Implementation ////////


bool HGE_Impl::sound_init() {
    if (!use_sound_ || hBass) {
        return true;
    }

    hBass = LoadLibrary("bass.dll");
    if (!hBass) {
        post_error("Can't load BASS.DLL");
        return false;
    }

    if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
        post_error("Incorrect BASS.DLL version");
        return false;
    }

    is_silent_ = false;
    if (!BASS_Init(-1, sample_rate_, 0, hwnd_, nullptr)) {
        System_Log("BASS Init failed, using no sound");
        BASS_Init(0, sample_rate_, 0, hwnd_, nullptr);
        is_silent_ = true;
    }
    else {
        BASS_DEVICEINFO info={};
		BASS_GetDeviceInfo(0,&info);
		System_Log("Sound Device: %s", info.name);
        System_Log("Sample rate: %ld\n", sample_rate_);
    }

    //BASS_SetConfig(BASS_CONFIG_BUFFER, 5000);
    //BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 50);

    set_fx_volume(fx_volume_);
    set_mus_volume(mus_volume_);
    set_stream_volume(stream_volume_);

    return true;
}

void HGE_Impl::sound_done() {
    auto stm_item = sound_streams_;

    if (hBass) {
        BASS_Stop();
        BASS_Free();

        //int err = BASS_ErrorGetCode();

        FreeLibrary(hBass);
        hBass = nullptr;

        while (stm_item) {
            const auto stm_next = stm_item->next;
            Resource_Free(stm_item->data);
            delete stm_item;
            stm_item = stm_next;
        }
        sound_streams_ = nullptr;
    }
}

void HGE_Impl::set_mus_volume(const int vol) const {
    if (!hBass) return;
    BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, vol * 100);
}

void HGE_Impl::set_stream_volume(const int vol) const {
    if (!hBass) return;
    BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, vol * 100);
}

void HGE_Impl::set_fx_volume(const int vol) const {
    if (!hBass) return;
    BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, vol * 100);
}
