#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "media_source.h"
#include "mpegts/packet.h"
#include "mpegts/pmt_builder.h"

#define PLAYER_PACKETS_AM 10

typedef enum
{
    PLYR_IDLE,
    PLYR_PLAYING,
    PLYR_PAUSED,
    PLYR_ERROR
} PlayerState_e;

typedef struct
{
    PlayerState_e state;
    MediaSource_t media_source;
    MpegTsPMTBuilder_t pmt_builder;
    MpegTsPacket_t packets[PLAYER_PACKETS_AM];
} Player_t;

bool player_init(Player_t *plyr);
void player_destroy(Player_t *plyr);

bool player_start(Player_t *plyr);
bool player_stop(Player_t *plyr);
bool player_process(Player_t *plyr);

bool player_replace_media_source_url(Player_t *plyr, char* new_url);

PlayerState_e player_get_state(Player_t *plyr);

