#
# Makefile
#

all: server client

%.o: %.c
	gcc -Wall -c $<

server: server.o alsa.o
	gcc -o audio_mix_server server.o alsa.o -lasound -lm

client: client.o audio_mix.o
	gcc -o client client.o audio_mix.o -lasound -lm

.PHONY: clean
clean:
	rm *.o server client librespot_client
