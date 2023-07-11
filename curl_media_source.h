#pragma once

#include <curl/curl.h>

#include "media_source.h"

#define MEDIA_SOURCE_CURL_MAX_URL_LEN          128
#define MEDIA_SOURCE_SCHEME_MAX_LEN            10
#define MEDIA_SOURCE_CURL_TRANSFER_BUFFER_SIZE (1024 * 4)

typedef struct
{
    CURLM *multi_handle;
    CURL *easy_handle;

    size_t transfer_buffer_write_offset;
    uint8_t curl_transfer_buffer[MEDIA_SOURCE_CURL_TRANSFER_BUFFER_SIZE];
} CURLMediaSource_t;

bool media_source_curl_from_raw(MediaSource_t *raw_source, CURLMediaSource_t **output);
bool media_source_curl_init(MediaSource_t *source);
bool media_source_curl_destroy(MediaSource_t *source);
bool media_source_curl_try_set_url(CURLMediaSource_t *source, char *new_url);

size_t media_source_curl_recv_packets(CURLMediaSource_t *source, MpegTsPacket_t *output_packets,
    size_t n_packets);
