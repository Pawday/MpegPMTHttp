#include <assert.h>
#include <curl/urlapi.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "media_source.h"
#include "mpegts/packet_inplace_parser.h"
#include "udp_media_source.h"

static bool is_udp_source(MediaSource_t *raw_source)
{
    if (raw_source == NULL) {
        return false;
    }

    if (raw_source->type != MEDIA_SOURCE_UDP) {
        return false;
    }

    if (raw_source->impl_data_size != sizeof(UDPMediaSource_t)) {
        return false;
    }

    return true;
}

bool media_source_udp_from_raw(MediaSource_t *raw_source, UDPMediaSource_t **output)
{
    if (!is_udp_source(raw_source)) {
        return false;
    }

    *output = raw_source->impl_data;
    return true;
}

bool media_source_udp_init(MediaSource_t *raw_source)
{
    if (raw_source == NULL) {
        return false;
    }

    bool raw_source_is_empty = true;
    raw_source_is_empty &= raw_source->type == MEDIA_SOURCE_NOTSET;
    raw_source_is_empty &= raw_source->impl_data == NULL;
    raw_source_is_empty &= raw_source->impl_data_size == 0;

    // Probably socket and memleak
    assert(raw_source_is_empty);
    if (!raw_source_is_empty) {
        return false;
    }

    UDPMediaSource_t *impl = malloc(sizeof(UDPMediaSource_t));
    if (impl == NULL) {
        return false;
    }

    impl->socket_fd = 0;
    memset(impl->udp_recv_buffer, 0, MEDIA_SOURCE_UDP_BUFF_SIZE);
    memset(&impl->udp_addres, 0, sizeof(impl->udp_addres));

    raw_source->impl_data = impl;
    raw_source->impl_data_size = sizeof(UDPMediaSource_t);
    raw_source->type = MEDIA_SOURCE_UDP;

    return true;
}

bool media_source_udp_destroy(MediaSource_t *raw_source)
{
    UDPMediaSource_t *impl = NULL;
    if (!media_source_udp_from_raw(raw_source, &impl)) {
        return false;
    }

    close(impl->socket_fd);
    free(impl);

    raw_source->type = MEDIA_SOURCE_NOTSET;
    raw_source->impl_data_size = 0;
    raw_source->impl_data = NULL;

    return true;
}

bool parse_udp_addr_from_url(struct sockaddr_in *output_addr, char *url)
{
    CURLU *url_parser = curl_url();
    if (!url_parser) {
        return false;
    }

    size_t url_len = strnlen(url, MEDIA_SOURCE_UDP_URL_MAX_SIZE);
    if (url_len == MEDIA_SOURCE_UDP_URL_MAX_SIZE) {
        goto exit_fail;
    }

    if (CURLE_OK != curl_url_set(url_parser, CURLUPART_URL, url, CURLU_NON_SUPPORT_SCHEME)) {
        goto exit_fail;
    }

    char *scheme = NULL;
    if (CURLE_OK != curl_url_get(url_parser, CURLUPART_SCHEME, &scheme, CURLU_NON_SUPPORT_SCHEME)) {
        goto exit_fail;
    }
    if (strcmp(scheme, "udp") != 0) {
        curl_free(scheme);
        goto exit_fail;
    }
    curl_free(scheme);

    char *addr_str = NULL;
    if (CURLE_OK != curl_url_get(url_parser, CURLUPART_HOST, &addr_str, 0)) {
        goto exit_fail;
    }

    struct in_addr addr;
    if (inet_aton(addr_str, &addr) == 0) {
        curl_free(addr_str);
        goto exit_fail;
    }
    curl_free(addr_str);

    in_port_t port;
    char *port_str = NULL;
    if (CURLE_OK != curl_url_get(url_parser, CURLUPART_PORT, &port_str, 0)) {
        goto exit_fail;
    }
    port = htons(atoi(port_str));
    curl_free(port_str);

    memset(output_addr, 0, sizeof(struct sockaddr_in));
    output_addr->sin_family = AF_INET;
    output_addr->sin_addr = addr;
    output_addr->sin_port = port;

    bool parse_status = true;
    goto exit_cleanup;
exit_fail:
    parse_status = false;
exit_cleanup:
    curl_url_cleanup(url_parser);
    return parse_status;
}

bool media_source_udp_try_set_url(UDPMediaSource_t *source, char *new_url)
{
    struct sockaddr_in new_addr = {0};
    if (!parse_udp_addr_from_url(&new_addr, new_url)) {
        return false;
    }

    if (memcmp(&source->udp_addres, &new_addr, sizeof(struct sockaddr_in)) == 0) {
        return false;
    }

    int new_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (new_socket < 0) {
        return false;
    }

    struct ip_mreq group_request = {0};
    group_request.imr_multiaddr.s_addr = new_addr.sin_addr.s_addr;
    group_request.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(new_socket,
            IPPROTO_IP,
            IP_ADD_MEMBERSHIP,
            &group_request,
            sizeof(group_request)) != 0) {
        close(new_socket);
        return false;
    }

    if (bind(new_socket, (struct sockaddr *)&new_addr, sizeof(struct sockaddr_in)) != 0) {
        close(new_socket);
        return false;
    }

    close(source->socket_fd);
    source->socket_fd = new_socket;
    return true;
}

size_t media_source_udp_recv_packets(UDPMediaSource_t *source, MpegTsPacket_t *output_packets,
    size_t n_packets)
{
    ssize_t bytes_recvd_or_err =
        recv(source->socket_fd, source->udp_recv_buffer, MEDIA_SOURCE_UDP_BUFF_SIZE, MSG_DONTWAIT);
    if (bytes_recvd_or_err < 0) {
        return 0;
    }

    return mpeg_ts_parse_packets_inplace(source->udp_recv_buffer,
        bytes_recvd_or_err,
        output_packets,
        n_packets);
}
