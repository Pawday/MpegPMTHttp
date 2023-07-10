#include "media_source.h"

#include "curl_media_source.h"

size_t media_source_recv_packets(MediaSource_t *source, MpegTsPacket_t *output_packets,
    size_t n_packets)
{
    CURLMediaSource_t *curl_source = NULL;
    if (media_source_curl_from_raw(source, &curl_source)) {
        return media_source_curl_recv_packets(curl_source, output_packets, n_packets);
    }

    return 0;
}
