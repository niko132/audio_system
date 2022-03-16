#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include <math.h>

#include "audio_mix.h"

#define SOCKNAME "/var/run/unix_socket_test.sock"

int main()
{
    audio_mix_init();

    unsigned char buffer[441 * 2 * 6];
    int16_t *audioBuffer = (int16_t*)buffer;

    for (int i = 0; i < 441; i++) {
        double rad = i * 2.0 * M_PI * 1000.0 / 44100.0;

        double sinVal = sin(rad);

        int16_t val = (int16_t)(sinVal * 32767);

        audioBuffer[i * 6] = val;
        audioBuffer[i * 6 + 1] = val;
        audioBuffer[i * 6 + 2] = val;
        audioBuffer[i * 6 + 3] = val;
        audioBuffer[i * 6 + 4] = val;
        audioBuffer[i * 6 + 5] = val;
    }

    int aaa;

    while (1) {
        // aaa = send(s1, buffer, 441 * 2 * 6, 0);
        aaa = audio_mix_write("default", buffer, 441 * 2 * 6, 6, CHANNEL_MAP_DEFAULT);
        printf("printing ... %d\n", aaa);
    }

    return 0;
}
