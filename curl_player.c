#include <assert.h>
#include <curl/curl.h>
#include <curl/urlapi.h>
#include <stdlib.h>
#include <string.h>

#include "curl/multi.h"
#include "mpegts/packet_inplace_parser.h"
#include "mpegts/pmt_builder.h"
#include "mpegts/pmt_dumper.h"
#include "pmt_output.h"

#include "curl_player.h"
#include "player.h"

bool is_url_source(MediaSource_t *source)
{
    if (source == NULL) {
        return false;
    }

    if (source->impl_data_size != sizeof(CURLSource_t)) {
        return false;
    }

    return true;
}

static inline bool url_source_impl(MediaSource_t *raw_source, CURLSource_t **output_source)
{
    if (!is_url_source(raw_source)) {
        return false;
    }

    *output_source = raw_source->impl_data;

    return true;
}

bool make_curl_source(MediaSource_t *output_source)
{
    CURLU *url = curl_url();
    if (url == NULL) {
        return false;
    }

    output_source->impl_data_size = sizeof(CURLSource_t);
    output_source->impl_data = malloc(output_source->impl_data_size);

    CURLSource_t *impl;
    bool get_impl_status = url_source_impl(output_source, &impl);
    assert(get_impl_status);
    if (!get_impl_status) {
        return false;
    }

    impl->curl_url_handle = url;

    return true;
}

bool replace_url(MediaSource_t *raw_source, const char *new_url)
{
    size_t url_len = strnlen(new_url, URL_MAX_LEN);
    if (url_len >= URL_MAX_LEN) {
        return false;
    }

    CURLSource_t *source;
    if (!url_source_impl(raw_source, &source)) {
        return false;
    }

    if (CURLUE_OK != curl_url_set(source->curl_url_handle, CURLUPART_URL, new_url, 0)) {
        return false;
    }

    return true;
}

bool cleanup_curl_source(MediaSource_t *media_source)
{
    CURLSource_t *impl;
    if (!url_source_impl(media_source, &impl)) {
        return false;
    }

    curl_url_cleanup(impl->curl_url_handle);
    free(media_source->impl_data);
    media_source->impl_data_size = 0;

    return true;
}

static size_t curl_data_chunk_recv_handler(void *data, size_t size, size_t nmemb,
    CurlPlayer_t *player)
{
    assert(size == 1);

    size_t parsed_packets = mpeg_ts_parse_packets_inplace(data,
        nmemb,
        player->packets,
        sizeof(player->packets) / sizeof(player->packets[0]));

    if (!process_packets(&player->pmt_builder, player->packets, parsed_packets)) {
        return 0;
    }

    return nmemb;
}

bool is_curl_player(Player_t *plyr)
{
    if (plyr == NULL) {
        return false;
    }

    if (plyr->impl_data == NULL) {
        return false;
    }

    if (plyr->impl_data_size != sizeof(CurlPlayer_t)) {
        return false;
    }

    return true;
}

static inline bool curl_plyr_get_impl(Player_t *plyr, CurlPlayer_t **out)
{
    if (!is_curl_player(plyr)) {
        return false;
    }

    *out = plyr->impl_data;

    return true;
}

bool player_curl_init(Player_t *plyr)
{
    if (plyr == NULL) {
        return false;
    }

    plyr->impl_data_size = sizeof(CurlPlayer_t);
    plyr->impl_data = malloc(plyr->impl_data_size);
    if (plyr->impl_data == NULL) {
        return false;
    }

    MediaSource_t url_source = {0};
    if (!make_curl_source(&url_source)) {
        goto error_free_impl_data;
    }

    CURLM *multi_handle = curl_multi_init();
    if (!multi_handle) {
        goto error_free_curl_source;
    }

    CURL *easy_handle = curl_easy_init();
    if (!easy_handle) {
        goto error_free_multi_handle;
    }

    uint8_t *pmt_build_buffer = malloc(MPEG_TS_PSI_PMT_SECTION_MAX_LENGTH);
    if (!pmt_build_buffer) {
        goto error_free_easy_handle;
    }

    CurlPlayer_t *plyr_impl = plyr->impl_data;
    plyr_impl->state = PLYR_IDLE;
    plyr_impl->last_curl_error_code = CURLE_OK;
    plyr_impl->media_source.impl_data = url_source.impl_data;
    plyr_impl->media_source.impl_data_size = sizeof(CURLSource_t);
    plyr_impl->multi_handle = multi_handle;
    plyr_impl->easy_handle = easy_handle;
    plyr_impl->pmt_build_buffer = pmt_build_buffer;

    mpeg_ts_pmt_builder_init(&plyr_impl->pmt_builder,
        pmt_build_buffer,
        MPEG_TS_PSI_PMT_SECTION_MAX_LENGTH);

    return true;

error_free_easy_handle:
    curl_easy_cleanup(easy_handle);
error_free_multi_handle:
    curl_multi_cleanup(multi_handle);
error_free_curl_source:
    cleanup_curl_source(&url_source);
error_free_impl_data:
    free(plyr->impl_data);
    plyr->impl_data = NULL;
    plyr->impl_data_size = 0;
    return false;
}

bool player_curl_destroy(Player_t *plyr)
{
    CurlPlayer_t *plyr_impl;
    if (!curl_plyr_get_impl(plyr, &plyr_impl)) {
        return false;
    }

    bool cleanup_status = cleanup_curl_source(&plyr_impl->media_source);
    assert(cleanup_status);
    if (!cleanup_status) {
        return false;
    }

    free(plyr_impl->pmt_build_buffer);
    curl_multi_cleanup(plyr_impl->multi_handle);
    curl_easy_cleanup(plyr_impl->easy_handle);
    free(plyr->impl_data);
    plyr->impl_data = NULL;
    plyr->impl_data_size = 0;

    return true;
}

bool player_get_media_source(Player_t *plyr, MediaSource_t **output_source)
{
    CurlPlayer_t *plyr_impl;
    if (!curl_plyr_get_impl(plyr, &plyr_impl)) {
        return false;
    }

    if (plyr_impl->state != PLYR_IDLE) {
        return false;
    }

    *output_source = &plyr_impl->media_source;
    return true;
}

bool player_start(Player_t *plyr)
{
    CurlPlayer_t *plyr_impl;
    if (!curl_plyr_get_impl(plyr, &plyr_impl)) {
        return false;
    }

    if (plyr_impl->state != PLYR_IDLE) {
        return false;
    }

    CURLSource_t *source = NULL;
    if (!url_source_impl(&plyr_impl->media_source, &source)) {
        return false;
    }

    if (CURLE_OK != curl_easy_setopt(plyr_impl->easy_handle,
                        CURLOPT_WRITEFUNCTION,
                        curl_data_chunk_recv_handler)) {
        return false;
    }

    if (CURLE_OK != curl_easy_setopt(plyr_impl->easy_handle, CURLOPT_WRITEDATA, plyr->impl_data)) {
        return false;
    }

    if (CURLE_OK !=
        curl_easy_setopt(plyr_impl->easy_handle, CURLOPT_CURLU, source->curl_url_handle)) {
        return false;
    }

    if (CURLM_OK != curl_multi_add_handle(plyr_impl->multi_handle, plyr_impl->easy_handle)) {
        return false;
    }

    plyr_impl->state = PLYR_PLAYING;

    return true;
}

bool player_stop(Player_t *plyr)
{
    CurlPlayer_t *plyr_impl;
    if (!curl_plyr_get_impl(plyr, &plyr_impl)) {
        return false;
    }

    if (plyr_impl->state != PLYR_PLAYING) {
        return false;
    }

    if (CURLM_OK != curl_multi_remove_handle(plyr_impl->multi_handle, plyr_impl->easy_handle)) {
        return false;
    }

    curl_easy_reset(plyr_impl->easy_handle);

    plyr_impl->state = PLYR_IDLE;

    return true;
}

PlayerState player_get_state(Player_t *plyr)
{
    CurlPlayer_t *plyr_impl;
    if (!curl_plyr_get_impl(plyr, &plyr_impl)) {
        return PLYR_ERROR;
    }

    return plyr_impl->state;
}

bool player_process(Player_t *plyr)
{
    CurlPlayer_t *plyr_impl;
    if (!curl_plyr_get_impl(plyr, &plyr_impl)) {
        return false;
    }

    if (plyr_impl->state != PLYR_PLAYING) {
        return false;
    }

    do {
        int handles_running = 0;
        switch (curl_multi_perform(plyr_impl->multi_handle, &handles_running)) {
        case CURLM_CALL_MULTI_PERFORM:
            continue;
        case CURLM_OK:
            return true;
        default:
            return false;
        }
    } while (false);

    return true;
}
