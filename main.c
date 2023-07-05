#include <assert.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "mpegts/packet_inplace_parser.h"
#include "mpegts/pmt_dumper.h"

void build_table_print_it_then_exit(MpegTsPMTBuilder_t *builder)
{

    if (builder->state != PMT_BUILDER_STATE_TABLE_ASSEMBLED) {
        exit(EXIT_FAILURE);
    }

    MpegTsPMT_t table = {0};
    if (!mpeg_ts_pmt_builder_try_build_table(builder, &table)) {
        exit(EXIT_FAILURE);
    }

    mpeg_ts_dump_pmt_to_stream(&table, stdout);
    printf("\n");

    exit(EXIT_SUCCESS);
}

size_t curl_data_chunk_recv_handler(void *data, size_t size, size_t nmemb, PMTParseKit_t *parse_kit)
{
    assert(size == 1);

    size_t parsed_packets = mpeg_ts_parse_packets_inplace(data,
        nmemb,
        parse_kit->packets,
        sizeof(parse_kit->packets) / sizeof(parse_kit->packets[0]));

    for (size_t packet_index = 0; packet_index < parsed_packets; packet_index++) {
        switch (mpeg_ts_pmt_builder_try_send_packet(&parse_kit->pmt_builder,
            parse_kit->packets + packet_index)) {
        case PMT_BUILDER_SEND_STATUS_TABLE_IS_ASSEMBLED:
        case PMT_BUILDER_SEND_STATUS_REDUNDANT_PACKET_REJECTED:
            build_table_print_it_then_exit(&parse_kit->pmt_builder);
        case PMT_BUILDER_SEND_STATUS_NEED_MORE_PACKETS:
        case PMT_BUILDER_SEND_STATUS_INVALID_PACKET_REJECTED:
            continue;
        case PMT_BUILDER_SEND_STATUS_UNORDERED_PACKET_REJECTED:
            mpeg_ts_pmt_builder_reset(&parse_kit->pmt_builder);
            continue;
        case PMT_BUILDER_SEND_STATUS_NOT_ENOUGHT_MEMORY:
            exit(EXIT_FAILURE);
            break;
        }
    }

    return nmemb;
}

int main()
{
    int exit_code = EXIT_SUCCESS;
    CURL *curl = curl_easy_init();
    PMTParseKit_t parse_kit;

    uint8_t pmt_build_buffer[MPEG_TS_PSI_PMT_SECTION_MAX_LENGTH];
    mpeg_ts_pmt_builder_init(&parse_kit.pmt_builder, pmt_build_buffer, sizeof(pmt_build_buffer));

    if (!curl) {
        printf("Error setup curl\n");
        goto exit_after_close_curl;
    }

    if (curl_easy_setopt(curl, CURLOPT_URL, TS_FILE_URL) != CURLE_OK) {
        printf("Error set %s addres\n", TS_FILE_URL);
        goto error_exit_clean_curl;
    }

    if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_data_chunk_recv_handler) != CURLE_OK) {
        printf("Error set data handler\n");
        goto error_exit_clean_curl;
    }
    if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, &parse_kit) != CURLE_OK) {
        printf("Error set parse kit as arg for curl's data handler\n");
        goto error_exit_clean_curl;
    }

    if (curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, MPEG_TS_PACKET_SIZE * 5) != CURLE_OK) {
        printf("Error set curl buffer size\n");
        goto error_exit_clean_curl;
    }

    CURLcode perform_status = curl_easy_perform(curl);
    if (perform_status != CURLE_OK) {
        printf("%s\n", curl_easy_strerror(perform_status));
        goto error_exit_clean_curl;
    }

    goto exit_clean_curl;

error_exit_clean_curl:
    exit_code = EXIT_FAILURE;
exit_clean_curl:
    curl_easy_cleanup(curl);
exit_after_close_curl:
    return exit_code;
}
