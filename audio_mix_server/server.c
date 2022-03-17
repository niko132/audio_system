#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/ioctl.h>

#include <alsa/asoundlib.h>
#include "alsa.h"

#include <math.h>

#define SOCKNAME "/var/run/unix_socket_test.sock"

#define NUM_SOCKETS 10
#define BYTES_PER_SAMPLE 2
#define NUM_CHANNELS 6
#define NUM_FRAMES_PER_LOOP 441 // 10ms
#define BYTES_PER_LOOP (BYTES_PER_SAMPLE * NUM_CHANNELS * NUM_FRAMES_PER_LOOP)
#define DELETE_SAMPLE_FAC 6

#define ALSA_DEVICE "default51"


int findFreeIndex(int *files, size_t len) {
    int i;
    for (i = 0; i < len; i += 1) {
        if (files[i] == -1) {
            return i;
        }
    }
    return -1;
}

int acceptClientSocket(int serverSocket, int *files, size_t len) {
    int clientSocket;
    struct sockaddr_un sa_client;
    socklen_t sa_len = sizeof sa_client;

    clientSocket = accept(serverSocket, (struct sockaddr *)&sa_client, &sa_len);
    if (clientSocket < 0 && errno != EWOULDBLOCK) {
        printf("E: %s\n", strerror(errno));
        return -1;
    } else if (clientSocket >= 0) {
        // set read to non blocking
        fcntl(clientSocket, F_SETFL, O_NONBLOCK);

        int index = findFreeIndex(files, len);
        printf("Free index: %d\n", index);

        if (index == -1) {
            close(clientSocket);
            return -1;
        }

        files[index] = clientSocket;
    }

    return 0;
}

void addSocketData(unsigned char *audioBuf, unsigned char *socketBuf, size_t audioSize, int *files, size_t len) {
    int16_t *audioSampleBuf = (int16_t*)audioBuf; // convert audio buffer to 16 bit aka 2 bytes per sample buffer
    int16_t *socketSampleBuf = (int16_t*)socketBuf;
    int i, j;
    ssize_t num;

    for (i = 0; i < len; i += 1) {
        if (files[i] == -1) {
            continue;
        }


        size_t bytesAvailable;
        ioctl(files[i], FIONREAD, &bytesAvailable);

//        printf("Bytes available: %d\n", bytesAvailable);

        if (bytesAvailable > BYTES_PER_LOOP * DELETE_SAMPLE_FAC) {
            size_t tooMuch = bytesAvailable - BYTES_PER_LOOP * DELETE_SAMPLE_FAC;

            if (tooMuch > BYTES_PER_SAMPLE * NUM_CHANNELS * 5) {
                tooMuch = BYTES_PER_SAMPLE * NUM_CHANNELS * 5;
            }

            size_t samplesTooMuch = (size_t)ceil((double)tooMuch / BYTES_PER_SAMPLE / NUM_CHANNELS);
            ssize_t deleteBytes = samplesTooMuch * BYTES_PER_SAMPLE * NUM_CHANNELS;

            printf("Deleted %d bytes (%fms)\n", deleteBytes, deleteBytes / 2.0 / 6.0 / 44.1);

            unsigned char deleteBuffer[1024];

            while (deleteBytes > 0) {
                size_t count = 1024;
                if (deleteBytes < count) {
                    count = deleteBytes;
                }
                size_t readBytes = read(files[i], deleteBuffer, count);
                deleteBytes = deleteBytes - readBytes;
            }
        }

        num = read(files[i], socketSampleBuf, audioSize);
        // TODO: do error handling

        if (num <= 0) {
            printf("Error: %d %d\n", num, errno);
            continue;
        }

        num = num / sizeof(int16_t); // convert from bytes to samples
        for (j = 0; j < num; j += 1) {
            audioSampleBuf[j] += socketSampleBuf[j]; // add contributions to global buffer
        }
    }
}

void cleanClosedSockets(int *files, size_t len) {
    int i, err;
    char c;

    for (i = 0; i < len; i += 1) {
        if (files[i] == -1) {
            continue;
        }

        err = recv(files[i], &c, 1, MSG_PEEK);
        if (err == 0) {
            printf("Stream #%d closed\n", i);
            files[i] = -1; // mark as closed and free
        } else if (err == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
            printf("Error reading stream #%d: %d %d\n", i, err, errno);
        }
    }
}


int main()
{
    int s1;
    struct sockaddr_un sa = { AF_UNIX, SOCKNAME };

    unlink(SOCKNAME);
    s1 = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s1 < 0) {
        printf("E: %s\n", strerror(errno));
    }

    // set accept to non blocking
    fcntl(s1, F_SETFL, O_NONBLOCK);
    bind(s1, (struct sockaddr *)&sa, sizeof sa);
    listen(s1, 10);


    int files[NUM_SOCKETS];
    memset(files, -1, NUM_SOCKETS * sizeof(int));


    unsigned char *audioBuf = (unsigned char*)malloc(BYTES_PER_LOOP);
    unsigned char *socketBuf = (unsigned char*)malloc(BYTES_PER_LOOP);
    receiver_data_t rec_data;

    rec_data.format.sample_rate = 129;
    rec_data.format.sample_size = BYTES_PER_SAMPLE * 8;
    rec_data.format.channels = NUM_CHANNELS;
    rec_data.format.channel_map = 6; // maybe change later
    rec_data.audio = audioBuf;
    rec_data.audio_size = BYTES_PER_LOOP; // 10 ms

    int latency = 0;
    alsa_output_init(latency, ALSA_DEVICE);

    while (1) {
        cleanClosedSockets(files, NUM_SOCKETS);
        acceptClientSocket(s1, files, NUM_SOCKETS);

        snd_pcm_sframes_t num_frames = alsa_frames_avail();

        if (num_frames > NUM_FRAMES_PER_LOOP || snd_pcm_state(alsa_get_pcm()) != SND_PCM_STATE_RUNNING) {
            memset(audioBuf, 0, BYTES_PER_LOOP); // set audio buf to zeros -> silence
            addSocketData(audioBuf, socketBuf, BYTES_PER_LOOP, files, NUM_SOCKETS);

            alsa_output_send(&rec_data);
        }
    }

    free(audioBuf);
    free(socketBuf);

    return 0;
}
