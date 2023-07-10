#pragma once

#include <stddef.h>

#include "mpegts/packet.h"

typedef struct
{
    void *impl_data;
    size_t impl_data_size;
} MediaSource_t;


size_t media_source_recv_packets(MediaSource_t *source, MpegTsPacket_t *output_packets, size_t n_packets);
void media_source_destroy(MediaSource_t *source);
