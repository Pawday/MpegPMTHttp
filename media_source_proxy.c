#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <curl/urlapi.h>

#include "curl_media_source.h"
#include "media_source.h"
#include "udp_media_source.h"

size_t media_source_recv_packets(MediaSource_t *source, MpegTsPacket_t *output_packets,
    size_t n_packets)
{
    switch (source->type) {

    case MEDIA_SOURCE_HTTP: {
        CURLMediaSource_t *curl_source = NULL;
        if (media_source_curl_from_raw(source, &curl_source)) {
            return media_source_curl_recv_packets(curl_source, output_packets, n_packets);
        }
        return 0;
    }
    case MEDIA_SOURCE_UDP: {
        UDPMediaSource_t *udp_source = NULL;
        if (media_source_udp_from_raw(source, &udp_source)) {
            return media_source_udp_recv_packets(udp_source, output_packets, n_packets);
        }
        return 0;
    }
    case MEDIA_SOURCE_NOTSET:
    case MEDIA_SOURCE_UNKNOWN:
        return 0;
    }

    assert(false && "Should be unreachable");
    return 0;
}

static MediaSourceType_e detect_media_source_type_from_url(char *url)
{
    MediaSourceType_e ret = MEDIA_SOURCE_UNKNOWN;

    // TODO: Parse handle can be global
    CURLU *url_parser = curl_url();
    if (url_parser == NULL) {
        ret = MEDIA_SOURCE_UNKNOWN;
        goto cleanup_parser;
    }

    if (CURLE_OK != curl_url_set(url_parser, CURLUPART_URL, url, CURLU_NON_SUPPORT_SCHEME)) {
        ret = MEDIA_SOURCE_UNKNOWN;
        goto cleanup_parser;
    }

    char *scheme = NULL;
    if (CURLE_OK != curl_url_get(url_parser, CURLUPART_SCHEME, &scheme, CURLU_NON_SUPPORT_SCHEME)) {
        ret = MEDIA_SOURCE_UNKNOWN;
        goto cleanup_scheme;
    }

    size_t scheme_len = strnlen(scheme, MEDIA_SOURCE_SCHEME_MAX_LEN);
    if (scheme_len == MEDIA_SOURCE_SCHEME_MAX_LEN) {
        ret = MEDIA_SOURCE_UNKNOWN;
        goto cleanup_scheme;
    }

    if (strcmp(scheme, "http") == 0) {
        ret = MEDIA_SOURCE_HTTP;
        goto cleanup_scheme;
    }

    if (strcmp(scheme, "udp") == 0) {
        ret = MEDIA_SOURCE_UDP;
        goto cleanup_scheme;
    }

cleanup_scheme:
    curl_free(scheme);
cleanup_parser:
    curl_url_cleanup(url_parser);
    return ret;
}

static bool is_source_empty(MediaSource_t *source)
{
    assert(source != NULL);

    bool is_empty = true;

    is_empty &= source->type == MEDIA_SOURCE_NOTSET;
    is_empty &= source->impl_data == NULL;
    is_empty &= source->impl_data_size == 0;

    return is_empty;
}

bool media_source_try_set_url(MediaSource_t *source, char *new_url)
{
    MediaSourceType_e detected_media_source = detect_media_source_type_from_url(new_url);
    switch (detected_media_source) {
    case MEDIA_SOURCE_UNKNOWN:
        return false;
    case MEDIA_SOURCE_HTTP: {
        if (source->type == MEDIA_SOURCE_NOTSET && !media_source_curl_init(source)) {
            return false;
        } else if (source->type == MEDIA_SOURCE_NOTSET) {
            source->type = MEDIA_SOURCE_HTTP;
        }

        if (source->type != MEDIA_SOURCE_HTTP) {
            if (source->type != MEDIA_SOURCE_NOTSET) {
                media_source_destroy(source);
            }

            if (!media_source_curl_init(source)) {
                return false;
            }
            source->type = MEDIA_SOURCE_HTTP;
        }
        CURLMediaSource_t *curl_source = NULL;
        if (!media_source_curl_from_raw(source, &curl_source)) {
            return false;
        }
        return media_source_curl_try_set_url(curl_source, new_url);
    }
    case MEDIA_SOURCE_UDP: {
        if (source->type == MEDIA_SOURCE_NOTSET && !media_source_udp_init(source)) {
            return false;
        }

        if (source->type != MEDIA_SOURCE_UDP) {
            if (source->type != MEDIA_SOURCE_NOTSET) {
                media_source_destroy(source);
            }

            if (!media_source_udp_init(source)) {
                return false;
            }
        }

        UDPMediaSource_t *udp_source = NULL;
        if (!media_source_udp_from_raw(source, &udp_source)) {
            return false;
        }

        return media_source_udp_try_set_url(udp_source, new_url);
    }

    case MEDIA_SOURCE_NOTSET:
        break;
    }

    assert(false && "Should be unreachable");
    return false;
}

void media_source_destroy(MediaSource_t *source)
{
    switch (source->type) {

    case MEDIA_SOURCE_NOTSET:
        assert(source->impl_data == NULL);
        assert(source->impl_data_size == 0);
        return;
    case MEDIA_SOURCE_HTTP: {
        bool curl_source_destroy_status = media_source_curl_destroy(source);
        assert(curl_source_destroy_status);
        assert(is_source_empty(source)); // No side effects
        return;
    }
    case MEDIA_SOURCE_UDP: {
        bool udp_source_destroy_status = media_source_udp_destroy(source);
        assert(udp_source_destroy_status);
        assert(is_source_empty(source)); // No side effects
        return;
    }
    case MEDIA_SOURCE_UNKNOWN:
        assert(false && "Looks like double free");
        return;
    }

    assert(false && "Should be unreachable");
    return;
}
