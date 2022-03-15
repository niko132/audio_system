#
# Makefile
#

all: server client librespot_client

%.o: %.c
	gcc -Wall -c $<

server: server.o alsa.o
	gcc -o server server.o alsa.o -lasound -lm

client: client.o audio_mix.o
	gcc -o client client.o audio_mix.o -lasound -lm

librespot_client: librespot_client.o audio_mix.o
	gcc -o librespot_client librespot_client.o audio_mix.o -lasound

.PHONY: clean
clean:
	rm *.o server client librespot_client
