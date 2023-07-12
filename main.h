#pragma once

#include <curl/curl.h>

#include "mpegts/packet.h"
#include "mpegts/pmt_builder.h"

#define PIPE_NAME "pmt_player_ctl"

#define IPC_MAX_MESSAGE_LEN 512
#define URL_MAX_LEN         64

typedef struct
{
    char url[URL_MAX_LEN + 1];
} PlayCommand_t;
