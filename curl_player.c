#include <assert.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#include "curl/multi.h"
#include "mpegts/packet_inplace_parser.h"
#include "mpegts/pmt_builder.h"
#include "mpegts/pmt_dumper.h"

#include "curl_player.h"
#include "player.h"

bool is_url_source(MediaSource_t *source)
{
    if (source == NULL) {
        return false;
    }

    if (source->impl_data_size != sizeof(UrlSource_t)) {
        return false;
    }

    return true;
}

static inline bool url_source_impl(MediaSource_t *raw_source, UrlSource_t **output_source)
{
    if (!is_url_source(raw_source)) {
        return false;
    }

    *output_source = raw_source->impl_data;

    return true;
}

bool make_url_source(MediaSource_t *output_source)
{
    output_source->impl_data_size = sizeof(UrlSource_t);
    output_source->impl_data = malloc(output_source->impl_data_size);

    UrlSource_t *impl;
    bool get_impl_status = url_source_impl(output_source, &impl);
    assert(get_impl_status);

    memset(impl->url, 0, URL_MAX_LEN);

    return true;
}

bool replace_url(MediaSource_t *raw_source, const char *new_url)
{
    UrlSource_t *source;
    if (!url_source_impl(raw_source, &source)) {
        return false;
    }

    size_t url_len = strnlen(new_url, URL_MAX_LEN);
    if (url_len == URL_MAX_LEN) {
        return false;
    }

    memcpy(source->url, new_url, url_len);
    source->url[url_len] = 0; // zero term

    return true;
}

bool delete_url_source(MediaSource_t *media_source)
{
    UrlSource_t *impl;
    if (!url_source_impl(media_source, &impl)) {
        return false;
    }

    media_source->impl_data_size = 0;
    free(impl);

    return true;
}

bool try_build_and_print_pmt(MpegTsPMTBuilder_t *builder)
{
    if (builder->state != PMT_BUILDER_STATE_TABLE_ASSEMBLED) {
        return false;
    }

    MpegTsPMT_t table = {0};
    if (!mpeg_ts_pmt_builder_try_build_table(builder, &table)) {
        return false;
    }

    mpeg_ts_dump_pmt_to_stream(&table, stdout);
    printf("\n");
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

    bool pmt_was_found = false;
    bool error_occurred = false;

    for (size_t packet_index = 0;
         !pmt_was_found && !error_occurred && packet_index < parsed_packets;
         packet_index++) {
        switch (mpeg_ts_pmt_builder_try_send_packet(&player->pmt_builder,
            player->packets + packet_index)) {
        case PMT_BUILDER_SEND_STATUS_TABLE_IS_ASSEMBLED:
        case PMT_BUILDER_SEND_STATUS_REDUNDANT_PACKET_REJECTED:
            if (try_build_and_print_pmt(&player->pmt_builder)) {
                pmt_was_found = true;
                break;
            }
            mpeg_ts_pmt_builder_reset(&player->pmt_builder);
            continue;
        case PMT_BUILDER_SEND_STATUS_NEED_MORE_PACKETS:
        case PMT_BUILDER_SEND_STATUS_INVALID_PACKET_REJECTED:
            continue;
        case PMT_BUILDER_SEND_STATUS_UNORDERED_PACKET_REJECTED:
            mpeg_ts_pmt_builder_reset(&player->pmt_builder);
            continue;
        case PMT_BUILDER_SEND_STATUS_NOT_ENOUGHT_MEMORY:
            error_occurred = true;
            break;
        }
    }

    if (error_occurred || pmt_was_found) {
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
    if (is_curl_player(plyr)) {
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

    CURLM *multi_handle = curl_multi_init();
    if (!multi_handle) {
        goto error_free_impl_data;
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
    plyr_impl->multi_handle = multi_handle;
    plyr_impl->easy_handle = easy_handle;
    plyr_impl->pmt_build_buffer = pmt_build_buffer;
    memset(plyr_impl->current_media_source.url, 0, URL_MAX_LEN);
    mpeg_ts_pmt_builder_init(&plyr_impl->pmt_builder,
        pmt_build_buffer,
        MPEG_TS_PSI_PMT_SECTION_MAX_LENGTH);

    if (CURLE_OK !=
        curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, curl_data_chunk_recv_handler)) {
        goto error_free_build_buffer;
    }

    if (CURLE_OK != curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, plyr->impl_data)) {
        goto error_free_build_buffer;
    }

    curl_multi_add_handle(multi_handle, easy_handle);

    return true;

error_free_build_buffer:
    free(plyr_impl->pmt_build_buffer);
error_free_easy_handle:
    curl_easy_cleanup(easy_handle);
error_free_multi_handle:
    curl_multi_cleanup(multi_handle);
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

    free(plyr_impl->pmt_build_buffer);
    curl_multi_cleanup(plyr_impl->multi_handle);
    curl_multi_cleanup(plyr_impl->multi_handle);
    free(plyr->impl_data);
    plyr->impl_data_size = 0;
    plyr->impl_data = NULL;

    return true;
}

bool player_set_source(Player_t *plyr, MediaSource_t *new_source_raw)
{
    CurlPlayer_t *plyr_impl;
    if (!curl_plyr_get_impl(plyr, &plyr_impl)) {
        return false;
    }

    if (plyr_impl->state != PLYR_IDLE) {
        return false;
    }

    UrlSource_t *new_source;
    if (!url_source_impl(new_source_raw, &new_source)) {
        return false;
    }

    plyr_impl->current_media_source = *new_source;

    if (CURLE_OK != curl_easy_setopt(plyr_impl->easy_handle,
                        CURLOPT_URL,
                        plyr_impl->current_media_source.url)) {
        return false;
    }

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

    return false;
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
