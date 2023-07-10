#include <stdlib.h>

#include "media_source.h"
#include "player.h"
#include "pmt_output.h"

bool player_init(Player_t *plyr)
{
    uint8_t *pmt_build_buffer = malloc(MPEG_TS_PSI_PMT_SECTION_MAX_LENGTH);
    if (pmt_build_buffer == NULL) {
        return false;
    }

    mpeg_ts_pmt_builder_init(&plyr->pmt_builder,
        pmt_build_buffer,
        MPEG_TS_PSI_PMT_SECTION_MAX_LENGTH);

    plyr->state = PLYR_IDLE;
    plyr->media_source = NULL;

    return true;
}

void player_destroy(Player_t *plyr)
{
    free(plyr->pmt_builder.table_data);
}

bool player_start(Player_t *plyr)
{
    if (plyr->state != PLYR_IDLE) {
        return false;
    }
    plyr->state = PLYR_PLAYING;
    return true;
}

bool player_stop(Player_t *plyr)
{
    if (plyr->state != PLYR_PLAYING) {
        return false;
    }
    plyr->state = PLYR_IDLE;
    return true;
}

bool player_process(Player_t *plyr)
{
    if (plyr->state != PLYR_PLAYING) {
        return false;
    }

    size_t packets_revcd =
        media_source_recv_packets(plyr->media_source, plyr->packets, PLAYER_PACKETS_AM);

    if (!pmt_output_process_packets(&plyr->pmt_builder, plyr->packets, packets_revcd)) {
        plyr->state = PLYR_ERROR;
        return false;
    }

    return true;
}

bool player_replace_media_source(Player_t *plyr, MediaSource_t *new_source)
{
    if (plyr->state != PLYR_IDLE) {
        return false;
    }

    pmt_output_reset_crc();
    plyr->media_source = new_source;

    return true;
}
