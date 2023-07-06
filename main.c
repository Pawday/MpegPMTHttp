#include <assert.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "mpegts/packet_inplace_parser.h"
#include "mpegts/pmt_builder.h"
#include "mpegts/pmt_dumper.h"

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

size_t curl_data_chunk_recv_handler(void *data, size_t size, size_t nmemb, PMTParseKit_t *parse_kit)
{
    assert(size == 1);

    size_t parsed_packets = mpeg_ts_parse_packets_inplace(data,
        nmemb,
        parse_kit->packets,
        sizeof(parse_kit->packets) / sizeof(parse_kit->packets[0]));

    bool pmt_was_found = false;
    bool error_occurred = false;

    for (size_t packet_index = 0;
         !pmt_was_found && !error_occurred && packet_index < parsed_packets;
         packet_index++) {
        switch (mpeg_ts_pmt_builder_try_send_packet(&parse_kit->pmt_builder,
            parse_kit->packets + packet_index)) {
        case PMT_BUILDER_SEND_STATUS_TABLE_IS_ASSEMBLED:
        case PMT_BUILDER_SEND_STATUS_REDUNDANT_PACKET_REJECTED:
            if (try_build_and_print_pmt(&parse_kit->pmt_builder)) {
                pmt_was_found = true;
                break;
            }
            mpeg_ts_pmt_builder_reset(&parse_kit->pmt_builder);
            continue;
        case PMT_BUILDER_SEND_STATUS_NEED_MORE_PACKETS:
        case PMT_BUILDER_SEND_STATUS_INVALID_PACKET_REJECTED:
            continue;
        case PMT_BUILDER_SEND_STATUS_UNORDERED_PACKET_REJECTED:
            mpeg_ts_pmt_builder_reset(&parse_kit->pmt_builder);
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

int main()
{
    CURL *curl = curl_easy_init();
    PMTParseKit_t parse_kit;

    uint8_t pmt_build_buffer[MPEG_TS_PSI_PMT_SECTION_MAX_LENGTH];
    mpeg_ts_pmt_builder_init(&parse_kit.pmt_builder, pmt_build_buffer, sizeof(pmt_build_buffer));

    if (!curl) {
        printf("Error setup curl\n");
        return EXIT_FAILURE;
    }

    if (curl_easy_setopt(curl, CURLOPT_URL, TS_STREAM_URL) != CURLE_OK) {
        printf("Error set %s address\n", TS_STREAM_URL);
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

    if (curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, CURL_RECV_BUFFER_SIZE) != CURLE_OK) {
        printf("Error set curl buffer size\n");
        goto error_exit_clean_curl;
    }

    CURLcode perform_status = curl_easy_perform(curl);

    // In that case CURLE_WRITE_ERROR means "we are done and you can terminate"
    // or "pmt builder no memory"
    if (perform_status != CURLE_WRITE_ERROR) {
        printf("%s\n", curl_easy_strerror(perform_status));
        goto error_exit_clean_curl;
    }

    curl_easy_cleanup(curl);
    return EXIT_SUCCESS;

error_exit_clean_curl:
    curl_easy_cleanup(curl);
    return EXIT_FAILURE;
}
