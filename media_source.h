#pragma once

#include <stddef.h>

#include "mpegts/packet.h"

typedef enum
{
    MEDIA_SOURCE_NOTSET,
    MEDIA_SOURCE_UNKNOWN,
    MEDIA_SOURCE_HTTP,
    MEDIA_SOURCE_UDP,
} MediaSourceType_e;

typedef struct
{
    MediaSourceType_e type;
    void *impl_data;
    size_t impl_data_size;
} MediaSource_t;

bool media_source_try_set_url(MediaSource_t *source, char *new_url);
size_t media_source_recv_packets(MediaSource_t *source, MpegTsPacket_t *output_packets,
    size_t n_packets);

void media_source_destroy(MediaSource_t *source);
