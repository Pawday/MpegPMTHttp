#pragma once

#include "mpegts/packet.h"
#include "mpegts/pmt_builder.h"

#define TS_FILE_URL "http://93.189.151.113:8754/udp/234.166.172.91:5555"

typedef struct
{
    MpegTsPacket_t packets[10];
    MpegTsPMTBuilder_t pmt_builder;
} PMTParseKit_t;
