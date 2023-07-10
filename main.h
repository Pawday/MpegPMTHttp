#pragma once

#include <curl/curl.h>

#include "mpegts/packet.h"
#include "mpegts/pmt_builder.h"

#define PIPE_NAME "pmt_player_ctl"

#define IPC_MAX_MESSAGE_LEN 512
#define PLAYER_CMD_MAX_LEN  (IPC_MAX_MESSAGE_LEN / 2)
#define URL_MAX_LEN         64

#define PACKETS_TO_RECV_IN_SINGLE_BUFFER 5
#define CURL_RECV_BUFFER_SIZE            (MPEG_TS_PACKET_SIZE * PACKETS_TO_RECV_IN_SINGLE_BUFFER)

typedef struct
{
    char url[URL_MAX_LEN + 1];
} PlayCommand;
