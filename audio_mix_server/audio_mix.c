#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include "audio_mix.h"

#define SOCKNAME "/var/run/unix_socket_test.sock"



static int sock_fd;


void audio_mix_init() {
  struct sockaddr_un sa = { AF_UNIX, SOCKNAME };

  sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  connect(sock_fd, (struct sockaddr *)&sa, sizeof sa);
}


size_t upmix_send(unsigned char *buffer, size_t len, int inChannels, int outChannels) {
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

  size_t ret = send(sock_fd, newBuffer, newLen, 0);
  free(newBuffer);
  return (size_t)(ret * (double)inChannels / outChannels);
}


size_t audio_mix_write(unsigned char *buffer, size_t len, int inChannels) {
  return upmix_send(buffer, len, inChannels, 6); // use always 6 channels
}


snd_pcm_sframes_t audio_mix_write_frames(unsigned char *buffer, snd_pcm_sframes_t frames, int inChannels) {
  int mult = 2 * inChannels; // use the default config TODO: maybe change later
  size_t bytes = frames * mult;
  size_t retBytes = audio_mix_write(buffer, bytes, inChannels);
  return retBytes / mult;
}

