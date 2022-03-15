#ifndef AUDIO_CLIENT_H
#define AUDIO_CLIENT_H

#include <alsa/asoundlib.h>

void audio_mix_init();
size_t audio_mix_write(unsigned char *buffer, size_t len, int inChannels);
snd_pcm_sframes_t audio_mix_write_frames(unsigned char *buffer, snd_pcm_sframes_t frames, int inChannels);

#endif
