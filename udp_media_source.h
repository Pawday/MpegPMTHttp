#pragma once

#include <arpa/inet.h>

#include "media_source.h"

#define MEDIA_SOURCE_UDP_BUFF_SIZE 1024

// That value is slightly bigger than strlen("udp://255.255.255.255:65535") whitch is 27
#define MEDIA_SOURCE_UDP_URL_MAX_SIZE 30

typedef struct
{
    int socket_fd;
    uint8_t udp_recv_buffer[MEDIA_SOURCE_UDP_BUFF_SIZE];
    struct sockaddr_in udp_addres;
} UDPMediaSource_t;

bool media_source_udp_from_raw(MediaSource_t *raw_source, UDPMediaSource_t **output);
bool media_source_udp_init(MediaSource_t *source);
bool media_source_udp_destroy(MediaSource_t *source);
bool media_source_udp_try_set_url(UDPMediaSource_t *source, char *new_url);

size_t media_source_udp_recv_packets(UDPMediaSource_t *source, MpegTsPacket_t *output_packets,
    size_t n_packets);
