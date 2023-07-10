#include "pmt_output.h"

#include "mpegts/pmt_dumper.h"

static uint32_t last_table_crc = 0;

static bool try_build_and_print_pmt(MpegTsPMTBuilder_t *builder)
{
    if (builder->state != PMT_BUILDER_STATE_TABLE_ASSEMBLED) {
        return false;
    }

    MpegTsPMT_t table = {0};
    if (!mpeg_ts_pmt_builder_try_build_table(builder, &table)) {
        return false;
    }

    if (table.CRC != last_table_crc) {
        mpeg_ts_dump_pmt_to_stream(&table, stdout);
        printf("\n");
    }

    return true;
}

bool process_packets(MpegTsPMTBuilder_t *builder, MpegTsPacket_t *packets, size_t packets_am)
{
    bool pmt_was_found = false;
    bool error_occurred = false;

    for (size_t packet_index = 0; !pmt_was_found && !error_occurred && packet_index < packets_am;
         packet_index++) {
        switch (mpeg_ts_pmt_builder_try_send_packet(builder, packets + packet_index)) {
        case PMT_BUILDER_SEND_STATUS_TABLE_IS_ASSEMBLED:
        case PMT_BUILDER_SEND_STATUS_REDUNDANT_PACKET_REJECTED:
            if (try_build_and_print_pmt(builder)) {
                pmt_was_found = true;
                break;
            }
            mpeg_ts_pmt_builder_reset(builder);
            continue;
        case PMT_BUILDER_SEND_STATUS_NEED_MORE_PACKETS:
        case PMT_BUILDER_SEND_STATUS_INVALID_PACKET_REJECTED:
            continue;
        case PMT_BUILDER_SEND_STATUS_UNORDERED_PACKET_REJECTED:
            mpeg_ts_pmt_builder_reset(builder);
            continue;
        case PMT_BUILDER_SEND_STATUS_NOT_ENOUGHT_MEMORY:
            error_occurred = true;
            break;
        }
    }
    if (pmt_was_found) {
        mpeg_ts_pmt_builder_reset(builder);
    }

    return !error_occurred;
}

