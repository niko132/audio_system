#include <stdio.h>
#include <unistd.h>

#include "audio_mix.h"

int main()
{
    audio_mix_init();

    unsigned char buffer[1024];

    size_t numRead;
    size_t numWrite;

    while (1) {
        numRead = read(STDIN_FILENO, buffer, 1024);

        if (numRead == 0) {
            break;
        }

        while (numRead > 0) {
            numWrite = audio_mix_write(buffer, numRead, 2, CHANNEL_MAP_DEFAULT);
            numRead = numRead - numWrite;
        }
    }

    return 0;
}
