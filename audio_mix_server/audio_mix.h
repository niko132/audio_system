#ifndef AUDIO_CLIENT_H
#define AUDIO_CLIENT_H

#include <alsa/asoundlib.h>

#define CHANNEL_MAP_DEFAULT 0
#define CHANNEL_MAP_DESKTOP 1
#define CHANNEL_MAP_LAPTOP 2

void audio_mix_init();
size_t audio_mix_write(unsigned char *buffer, size_t len, int inChannels, int map_id);
snd_pcm_sframes_t audio_mix_write_frames(unsigned char *buffer, snd_pcm_sframes_t frames, int inChannels, int map_id);

#endif
