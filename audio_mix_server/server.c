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

        printf("Added %d samples\n", num);
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
    listen(s1, 5);


    int files[NUM_SOCKETS];
    memset(files, -1, NUM_SOCKETS * sizeof(int));




    int latency = 25;
    char *alsa_device = "default51";

    alsa_output_init(latency, alsa_device);

    receiver_data_t rec_data;

    rec_data.format.sample_rate = 129;
    rec_data.format.sample_size = 16;
    rec_data.format.channels = 6;
    rec_data.format.channel_map = 6; // maybe change later



    size_t num_frames_write = 441;

    rec_data.audio_size = num_frames_write * 2 * 6; // 10 ms
    rec_data.audio = (unsigned char*)malloc(num_frames_write * 2 * 6);


    /*
    int16_t *sample_buf = (int16_t*)rec_data.audio;


    for (int a = 0; a < num_frames_write; a += 1) {
        double rad = a * 2 * M_PI * 1000.0 / 44100.0; // 1000Hz sine

        double sinVal = sin(rad);

        int16_t val = (int16_t)(sinVal * 32767);

        sample_buf[a * 6] = val;
        sample_buf[a * 6 + 1] = val;
        sample_buf[a * 6 + 2] = val;
        sample_buf[a * 6 + 3] = val;
        sample_buf[a * 6 + 4] = val;
        sample_buf[a * 6 + 5] = val;
    }


    for (int i = 0; i < 10; i += 1) {
        alsa_output_send(&rec_data);
    }

    snd_pcm_start(alsa_get_pcm());

    while (1) {
        snd_pcm_sframes_t num_frames = alsa_frames_avail();
        // snd_pcm_sframes_t num_frames = alsa_frames_delay();

        while (num_frames > num_frames_write) {
            alsa_output_send(&rec_data);
//            snd_pcm_start(alsa_get_pcm());
            num_frames = alsa_frames_avail();
            // num_frames = alsa_frames_delay();

            printf("written!\n");
        }

//        printf("Frames: %ld\n", num_frames);

        if (num_frames < 0) {
            break;
        }
    }
    */



    printf("Hello World\n");



    unsigned char *audioBuf = (unsigned char*)malloc(num_frames_write * 2 * 6); // hold 512 samples -> about 11.6ms of data
    unsigned char *socketBuf = (unsigned char*)malloc(num_frames_write * 2 * 6);



    rec_data.audio = audioBuf;




    for (int i = 0; i < 10; i += 1) {
        alsa_output_send(&rec_data);
    }

    snd_pcm_start(alsa_get_pcm());



    int i, err;
    size_t bytesAvailable;
    int shouldRead = 0;

    while (1) {
        cleanClosedSockets(files, NUM_SOCKETS);
        acceptClientSocket(s1, files, NUM_SOCKETS);


        /*
        shouldRead = 0;
        for (i = 0; i < NUM_SOCKETS; i += 1) {
            if (files[i] == -1) {
                continue;
            }

            err = ioctl(files[i], FIONREAD, &bytesAvailable);

            if (bytesAvailable >= num_frames_write * 2 * 6 * 2) {
                shouldRead = 1;
            }
        }

        if (shouldRead) {
            printf("Reading...\n");
            memset(audioBuf, 0, num_frames_write * 2 * 6); // set audio buf to zeros -> silence
            addSocketData(audioBuf, socketBuf, num_frames_write * 2 * 6, files, NUM_SOCKETS);

            printf("Data: %d %d %d %d %d\n", audioBuf[0], audioBuf[1], audioBuf[2], audioBuf[3], audioBuf[4]);
        }
        */



        snd_pcm_sframes_t num_frames = alsa_frames_avail();

//        while (num_frames > num_frames_write) {
        if (num_frames > num_frames_write) {
            memset(audioBuf, 0, num_frames_write * 2 * 6); // set audio buf to zeros -> silence
            addSocketData(audioBuf, socketBuf, num_frames_write * 2 * 6, files, NUM_SOCKETS);

            alsa_output_send(&rec_data);
            num_frames = alsa_frames_avail();

            printf("Written %ld %ld\n", num_frames, num_frames_write);
        }
    }


    free(audioBuf);
    free(socketBuf);

    return 0;
}
