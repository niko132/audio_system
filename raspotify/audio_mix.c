#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include "audio_mix.h"

#define SOCKNAME "/var/run/unix_socket_test.sock"

static int channel_maps[3][6] = {
  {0,1,4,5,2,3},
  {1,5,0,4,2,3},
  {0,1,4,5,2,3}
};

static double channel_facs[3][6] = {
  {1.0,1.0,1.0,1.0,1.0,1.0},
  {0.7,1.0,0.4,0.9,0.4,1.0},
  {1.0,0.9,0.9,0.7,0.4,1.0}
};


static int sock_fd;


void audio_mix_init() {
  struct sockaddr_un sa = { AF_UNIX, SOCKNAME };

  sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  connect(sock_fd, (struct sockaddr *)&sa, sizeof sa);
}


size_t apply_channel_map_send(unsigned char *buffer, size_t len, int map_id) {
  size_t num_samples = len / 6 / 2; // default config: 6 channels, 2 bytes per sample

  int16_t *sampleBuffer = (int16_t*)buffer;
  int16_t singleSample[6 * sizeof(int16_t)];

  for (int i = 0; i < num_samples; i++) {
    memcpy(singleSample, &sampleBuffer[i * 6], 6 * sizeof(int16_t));

    sampleBuffer[i * 6] = (int16_t)(singleSample[channel_maps[map_id][0]] * channel_facs[map_id][0]);
    sampleBuffer[i * 6 + 1] = (int16_t)(singleSample[channel_maps[map_id][1]] * channel_facs[map_id][1]);
    sampleBuffer[i * 6 + 2] = (int16_t)(singleSample[channel_maps[map_id][2]] * channel_facs[map_id][2]);
    sampleBuffer[i * 6 + 3] = (int16_t)(singleSample[channel_maps[map_id][3]] * channel_facs[map_id][3]);
    sampleBuffer[i * 6 + 4] = (int16_t)(singleSample[channel_maps[map_id][4]] * channel_facs[map_id][4]);
    sampleBuffer[i * 6 + 5] = (int16_t)(singleSample[channel_maps[map_id][5]] * channel_facs[map_id][5]);
  }


  return send(sock_fd, buffer, len, 0);
}


size_t upmix_send(unsigned char *buffer, size_t len, int inChannels, int outChannels, int map_id) {
  size_t newLen = (size_t)(len * (double)outChannels / inChannels);
  unsigned char *newBuffer = (unsigned char*)malloc(newLen);

  size_t numSamples = len / inChannels / 2;
  int16_t *sampleBuffer = (int16_t*)buffer;
  int16_t *newSampleBuffer = (int16_t*)newBuffer;

  for (int i = 0; i < numSamples; i++) {
    for (int j = 0; j < outChannels; j++) {
      newSampleBuffer[i * outChannels + j] = sampleBuffer[i * inChannels + j % inChannels];
    }
  }

  /*
  size_t ret = send(sock_fd, newBuffer, newLen, 0);
  free(newBuffer);
  return (size_t)(ret * (double)inChannels / outChannels);
  */

  size_t ret = apply_channel_map_send(newBuffer, newLen, map_id);
  free(newBuffer);
  return (size_t)(ret * (double)inChannels / outChannels);
}


size_t audio_mix_write(unsigned char *buffer, size_t len, int inChannels, int map_id) {
  return upmix_send(buffer, len, inChannels, 6, map_id); // use always 6 channels
}


snd_pcm_sframes_t audio_mix_write_frames(unsigned char *buffer, snd_pcm_sframes_t frames, int inChannels, int map_id) {
  int mult = 2 * inChannels; // use the default config TODO: maybe change later
  size_t bytes = frames * mult;
  size_t retBytes = audio_mix_write(buffer, bytes, inChannels, map_id);
  return retBytes / mult;
}

