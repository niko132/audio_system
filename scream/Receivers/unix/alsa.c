#include "alsa.h"
#include <stdbool.h>

#include "audio_mix.h"

static struct alsa_output_data {
  snd_pcm_chmap_t *channel_map;
  snd_pcm_t *snd;

  receiver_format_t receiver_format;
  unsigned int rate;
  unsigned int bytes_per_sample;

  int latency;
  char *alsa_device;
} ao_data;

int alsa_output_init(int latency, char *alsa_device)
{
  audio_mix_init();

  return 0;
}

int alsa_output_send(receiver_data_t *data)
{
  snd_pcm_format_t format;

  receiver_format_t *rf = &data->format;


  bool newFormat = memcmp(&ao_data.receiver_format, rf, sizeof(receiver_format_t));
  if (newFormat) {
    // audio format changed, reconfigure
    memcpy(&ao_data.receiver_format, rf, sizeof(receiver_format_t));

    ao_data.rate = ((rf->sample_rate >= 128) ? 44100 : 48000) * (rf->sample_rate % 128);
    switch (rf->sample_size) {
      case 16: format = SND_PCM_FORMAT_S16_LE; ao_data.bytes_per_sample = 2; break;
      case 24: format = SND_PCM_FORMAT_S24_3LE; ao_data.bytes_per_sample = 3; break;
      case 32: format = SND_PCM_FORMAT_S32_LE; ao_data.bytes_per_sample = 4; break;
      default:
        if (verbosity > 0)
          fprintf(stderr, "Unsupported sample size %hhu, not playing until next format switch.\n", rf->sample_size);
        ao_data.rate = 0;
    }
  }



  if (!ao_data.rate) return 0;

  int ret;
  snd_pcm_sframes_t written;

  int i = 0;
  int samples = (data->audio_size) / (ao_data.bytes_per_sample * rf->channels);
  while (i < samples) {
    written = audio_mix_write_frames(&data->audio[i * ao_data.bytes_per_sample * rf->channels], samples - i, rf->channels);

    if (written < 0) {
      return 0;
    } else if (written < samples - i) {
      if (verbosity) fprintf(stderr, "Writing again after short write %ld < %d\n", written, samples - i);
    }
    i += written;
  }

  return 0;
}
