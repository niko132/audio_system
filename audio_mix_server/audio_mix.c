#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <time.h>

#include "audio_mix.h"

#define SOCKNAME "/var/run/unix_socket_test.sock"
#define NUM_SOCKETS 10
#define INACTIVE_SECONDS 30

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


typedef struct {
  char *key;
  int socket;
  unsigned long last_query_time;
} key_socket_t;


static key_socket_t socket_map[NUM_SOCKETS];


void audio_mix_init() {
  // initialize the map structure with invalid values
  for (int i = 0; i < NUM_SOCKETS; i++) {
    socket_map[i].key = NULL,
    socket_map[i].socket = -1;
  }
}


void close_stream(char *key) {
  for (int i = 0; i < NUM_SOCKETS; i++) {
    if (socket_map[i].key == NULL) {
      continue;
    }

    if (strcmp(key, socket_map[i].key) == 0) {
      // free the memory of the key and close the socket
      free(socket_map[i].key);
      socket_map[i].key = NULL;
      close(socket_map[i].socket);
      socket_map[i].socket = -1;

      break;
    }
  }
}


int get_socket(char *key) {
  // recycle unused map entries
  unsigned long now_time = time(NULL);
  for (int i = 0; i < NUM_SOCKETS; i++) {
    if (socket_map[i].key == NULL) {
      continue;
    }

    if (now_time > socket_map[i].last_query_time + INACTIVE_SECONDS) {
      printf("Closing stream #%d: %s\n", i, socket_map[i].key);
      close_stream(socket_map[i].key);
    }
  }

  // search for the key in the map
  for (int i = 0; i < NUM_SOCKETS; i++) {
    if (socket_map[i].key != NULL && strcmp(key, socket_map[i].key) == 0) {
      socket_map[i].last_query_time = time(NULL);
      return socket_map[i].socket;
    }
  }

  // key not found -> create a new socket
  int next_index = -1;
  for (int i = 0; i < NUM_SOCKETS; i++) {
    if (socket_map[i].key == NULL && socket_map[i].socket < 0) {
      next_index = i;
      break;
    }
  }

  if (next_index >= 0) {
    struct sockaddr_un sa = { AF_UNIX, SOCKNAME };

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    connect(sock, (struct sockaddr *)&sa, sizeof sa);

    // insert into the map
    socket_map[next_index].key = (char*)malloc(strlen(key) + 1);
    strcpy(socket_map[next_index].key, key);
    socket_map[next_index].socket = sock;
    socket_map[next_index].last_query_time = time(NULL);

    return sock;
  }

  return -1;
}


size_t apply_channel_map_send(char *key, unsigned char *buffer, size_t len, int map_id) {
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


  int socket = get_socket(key);
  return send(socket, buffer, len, 0);
}


size_t upmix_send(char *key, unsigned char *buffer, size_t len, int inChannels, int outChannels, int map_id) {
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

  size_t ret = apply_channel_map_send(key, newBuffer, newLen, map_id);
  free(newBuffer);
  return (size_t)(ret * (double)inChannels / outChannels);
}


size_t audio_mix_write(char *key, unsigned char *buffer, size_t len, int inChannels, int map_id) {
  return upmix_send(key, buffer, len, inChannels, 6, map_id); // use always 6 channels
}


snd_pcm_sframes_t audio_mix_write_frames(char *key, unsigned char *buffer, snd_pcm_sframes_t frames, int inChannels, int map_id) {
  int mult = 2 * inChannels; // use the default config TODO: maybe change later
  size_t bytes = frames * mult;
  size_t retBytes = audio_mix_write(key, buffer, bytes, inChannels, map_id);
  return retBytes / mult;
}

