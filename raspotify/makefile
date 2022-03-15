#
# Makefile
#

all: librespot_client

%.o: %.c
	gcc -Wall -c $<

librespot_client: librespot_client.o audio_mix.o
	gcc -o librespot_client librespot_client.o audio_mix.o -lasound

.PHONY: clean
clean:
	rm *.o librespot_client
