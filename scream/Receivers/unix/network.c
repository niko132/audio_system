#include "network.h"
#include <stdio.h>

#include <sys/time.h>

#include "audio_mix.h"


static rctx_network_t rctx_network;

int init_network(enum receiver_type receiver_mode, in_addr_t interface, int port, char* multicast_group)
{
  rctx_network.sockfd = socket(AF_INET,SOCK_DGRAM,0);

  memset((void *)&(rctx_network.servaddr), 0, sizeof(rctx_network.servaddr));
  rctx_network.servaddr.sin_family = AF_INET;
  rctx_network.servaddr.sin_addr.s_addr = (receiver_mode == Unicast) ? interface : htonl(INADDR_ANY);
  rctx_network.servaddr.sin_port = htons(port);
  bind(rctx_network.sockfd, (struct sockaddr *)&rctx_network.servaddr, sizeof(rctx_network.servaddr));

  if (receiver_mode == Multicast) {
    rctx_network.imreq.imr_multiaddr.s_addr = inet_addr(multicast_group ? multicast_group : DEFAULT_MULTICAST_GROUP);
    rctx_network.imreq.imr_interface.s_addr = interface;

    setsockopt(rctx_network.sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
              (const void *)&rctx_network.imreq, sizeof(struct ip_mreq));
  }

  return 0;
}


static struct timeval start;
size_t totalRec = 0;

void rcv_network(receiver_data_t* receiver_data)
{
  ssize_t n = 0;


  struct sockaddr_in from;
  int fromlen = sizeof(struct sockaddr);


  while (n < HEADER_SIZE) {
    n = recvfrom(rctx_network.sockfd, rctx_network.buf, MAX_SO_PACKETSIZE, 0, (struct sockaddr*)&from, &fromlen);
  }



  // printf("Packet from %s (%u bytes)\n", inet_ntoa(from.sin_addr), n);

  if (strcmp("192.168.3.3", inet_ntoa(from.sin_addr)) == 0) {
    receiver_data->map_id = CHANNEL_MAP_DESKTOP;
  } else if (strcmp("192.168.3.4", inet_ntoa(from.sin_addr)) == 0) {
    receiver_data->map_id = CHANNEL_MAP_DESKTOP;
  } else if (strcmp("192.168.3.5", inet_ntoa(from.sin_addr)) == 0) {
    receiver_data->map_id = CHANNEL_MAP_LAPTOP;
  } else {
    receiver_data->map_id = CHANNEL_MAP_DEFAULT;
  }

  totalRec += n - 5; // subtract the header size

  struct timeval now;
  gettimeofday(&now, NULL);

  unsigned long diff = (now.tv_sec - start.tv_sec) * 1000 + (now.tv_usec - start.tv_usec) / 1000;

  if (diff >= 2000) { // last measurement is 1s ago
    double totalMsRec = totalRec * 1000 / 2.0 / 4.0 / 44100.0;
    printf("Received %.2fms\n", totalMsRec * 2000.0 / diff);

    start = now;
    totalRec = 0;
  }


  // fprintf(stdout, "Recv Packet: %d \n", n);

  receiver_data->format.sample_rate = rctx_network.buf[0];
  receiver_data->format.sample_size = rctx_network.buf[1];
  receiver_data->format.channels = rctx_network.buf[2];
  receiver_data->format.channel_map = (rctx_network.buf[4] << 8) | rctx_network.buf[3];
  receiver_data->audio_size = n - HEADER_SIZE;
  receiver_data->audio = &rctx_network.buf[5];
}

