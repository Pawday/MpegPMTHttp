#pragma once

#include "mpegts/pmt_builder.h"


bool pmt_output_process_packets(MpegTsPMTBuilder_t *builder, MpegTsPacket_t *packets, size_t packets_am);

void pmt_output_reset_crc(void); 
