#include <curl/curl.h>
#include <stdint.h>

#include "mpegts/packet.h"
#include "mpegts/pmt.h"
#include "mpegts/pmt_builder.h"

#include "player.h"

#define URL_MAX_LEN 256
#define PACKETS_AM 5

typedef struct
{
    uint8_t url[URL_MAX_LEN];
} UrlSource_t;

typedef struct
{
    PlayerState state;

    uint8_t *pmt_build_buffer;
    MpegTsPMTBuilder_t pmt_builder;
    MpegTsPacket_t packets[PACKETS_AM];

    CURLM *multi_handle;
    CURL *easy_handle;
    CURLcode last_curl_error_code;
    UrlSource_t current_media_source;
} CurlPlayer_t;

bool is_url_source(MediaSource_t *url_source);
bool make_url_source(MediaSource_t *output_source);
bool replace_url(MediaSource_t *source, const char* new_url);
bool delete_url_source(MediaSource_t *media_source);

bool is_curl_player(Player_t *plyr);
bool player_curl_init(Player_t *plyr);
bool player_curl_destroy(Player_t *plyr);

