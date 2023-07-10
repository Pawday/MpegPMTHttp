#pragma once

#include "mpegts/pmt_builder.h"


bool process_packets(MpegTsPMTBuilder_t *builder, MpegTsPacket_t *packets, size_t packets_am);
