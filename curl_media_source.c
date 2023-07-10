#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "curl_media_source.h"
#include "mpegts/packet_inplace_parser.h"

static bool is_curl_source(MediaSource_t *raw_source)
{
    if (raw_source->impl_data_size != sizeof(CURLMediaSource_t)) {
        return false;
    }

    return true;
}

bool media_source_curl_from_raw(MediaSource_t *raw_source, CURLMediaSource_t **output)
{
    if (!is_curl_source(raw_source)) {
        return false;
    }

    *output = raw_source->impl_data;
    return true;
}

size_t curl_revc_handler(char *data, size_t size, size_t nmemb, CURLMediaSource_t *source)
{
    assert(size == 1);

    if (source->transfer_buffer_write_offset > MEDIA_SOURCE_CURL_TRANSFER_BUFFER_SIZE) {
        return CURL_WRITEFUNC_ERROR;
    }

    size_t bytes_avail =
        MEDIA_SOURCE_CURL_TRANSFER_BUFFER_SIZE - source->transfer_buffer_write_offset;
    if (nmemb > bytes_avail) {
        return CURL_WRITEFUNC_PAUSE;
    }

    memcpy(source->curl_transfer_buffer + source->transfer_buffer_write_offset, data, nmemb);
    source->transfer_buffer_write_offset += nmemb;

    return nmemb;
}

static bool setup_easy_handle(CURLMediaSource_t *source)
{
    if (CURLE_OK !=
        curl_easy_setopt(source->easy_handle, CURLOPT_WRITEFUNCTION, curl_revc_handler)) {
        return false;
    }
    if (CURLE_OK != curl_easy_setopt(source->easy_handle, CURLOPT_WRITEDATA, source)) {
        return false;
    }

    return true;
}

bool media_source_curl_init(MediaSource_t *raw_source)
{
    if (raw_source == NULL) {
        return false;
    }

    CURL *easy_handle = curl_easy_init();
    if (!easy_handle) {
        return false;
    }

    CURLM *multi_handle = curl_multi_init();
    if (!multi_handle) {
        curl_easy_cleanup(easy_handle);
        return false;
    }

    if (CURLE_OK != curl_multi_add_handle(multi_handle, easy_handle)) {
        goto exit_error_cleanup_handles;
    }

    raw_source->impl_data = NULL;
    raw_source->impl_data_size = 0;
    CURLMediaSource_t *impl = malloc(sizeof(CURLMediaSource_t));
    if (impl == NULL) {
        goto exit_error_cleanup_handles;
    }

    impl->multi_handle = multi_handle;
    impl->easy_handle = easy_handle;
    impl->transfer_buffer_write_offset = 0;

    raw_source->impl_data = impl;
    raw_source->impl_data_size = sizeof(CURLMediaSource_t);

    return true;

exit_error_cleanup_handles:
    curl_easy_cleanup(easy_handle);
    curl_multi_cleanup(multi_handle);
    return false;
}

bool media_source_curl_destroy(MediaSource_t *source)
{
    CURLMediaSource_t *impl = NULL;
    if (!media_source_curl_from_raw(source, &impl)) {
        return false;
    }
    curl_easy_cleanup(impl->easy_handle);
    curl_multi_cleanup(impl->multi_handle);

    free(impl);

    return true;
}

bool media_source_curl_try_set_url(CURLMediaSource_t *source, char *new_url)
{
    size_t url_len = strnlen(new_url, MEDIA_SOURCE_CURL_MAX_URL_LEN);
    if (url_len == MEDIA_SOURCE_CURL_MAX_URL_LEN) {
        return false;
    }

    curl_easy_reset(source->easy_handle);
    if (!setup_easy_handle(source)) {
        return false;
    }

    if (CURLE_OK != curl_easy_setopt(source->easy_handle, CURLOPT_URL, new_url)) {
        return false;
    }

    return true;
}

size_t media_source_curl_recv_packets(CURLMediaSource_t *source, MpegTsPacket_t *output_packets,
    size_t n_packets)
{
    source->transfer_buffer_write_offset = 0;

    int handles = 0;
    curl_multi_perform(source->multi_handle, &handles);

    return mpeg_ts_parse_packets_inplace(source->curl_transfer_buffer,
        source->transfer_buffer_write_offset,
        output_packets,
        n_packets);
}
