#pragma once

#include "mpegts/packet.h"
#include "mpegts/pmt_builder.h"

#define TS_STREAM_URL                    "http://93.189.151.113:8754/udp/234.166.172.91:5555"

#define PACKETS_TO_RECV_IN_SINGLE_BUFFER 5
#define CURL_RECV_BUFFER_SIZE (MPEG_TS_PACKET_SIZE * PACKETS_TO_RECV_IN_SINGLE_BUFFER)

typedef struct
{
    MpegTsPacket_t packets[10];
    MpegTsPMTBuilder_t pmt_builder;
} PMTParseKit_t;
