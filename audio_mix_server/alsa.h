#ifndef ALSA_H
#define ALSA_H

#include <alsa/asoundlib.h>

#define MAX_CHANNELS 8

static int verbosity = 1;

typedef struct receiver_format {
  unsigned char sample_rate;
  unsigned char sample_size;
  unsigned char channels;
  uint16_t channel_map;
} receiver_format_t;

typedef struct receiver_data {
  receiver_format_t format;
  unsigned int audio_size;
  unsigned char* audio;
} receiver_data_t;

snd_pcm_sframes_t alsa_frames_avail();
snd_pcm_sframes_t alsa_frames_delay();
snd_pcm_t* alsa_get_pcm();

int alsa_output_init(int latency, char *alsa_device);
int alsa_output_send(receiver_data_t *data);

#endif
