#pragma once

#include "mpegts/packet.h"
#include "mpegts/pmt_builder.h"

#define TS_FILE_URL "http://home.imaqliq.tv/Katy8.ts"

typedef struct
{
    MpegTsPacket_t packets[10];
    MpegTsPMTBuilder_t pmt_builder;
} PMTParseKit_t;
