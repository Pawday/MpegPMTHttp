#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <json-c/json_object.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <json-c/json.h>

#include "curl_player.h"
#include "main.h"
#include "mpegts/packet_inplace_parser.h"
#include "mpegts/pmt_builder.h"
#include "mpegts/pmt_dumper.h"
#include "player.h"

static json_tokener *main_tokener;

bool parse_source_from_json(UrlSource_t *output_source, uint8_t *buff, size_t len)
{
    if (len > INT32_MAX) {
        return false;
    }

    json_object *obj = json_tokener_parse_ex(main_tokener, (char *)buff, len);
    if (!obj) {
        return false;
    }

    bool parse_status = true;

    json_object *cmd;
    if (parse_status && !json_object_object_get_ex(obj, "cmd", &cmd)) {
        parse_status = false;
    }

    json_object *url;
    if (parse_status && !json_object_object_get_ex(obj, "url", &url)) {
        parse_status = false;
    }

    json_object_put(obj);
    return parse_status;
}

int main(void)
{
    if (mkfifo(PIPE_NAME, 0777) != 0 && errno != EEXIST) {
        perror("Error making pipe");
        return EXIT_FAILURE;
    }

    int pipe_fd = open(PIPE_NAME, O_RDWR | O_CREAT, 0777);
    if (0 > pipe_fd) {
        perror("Error open pipe");
        return EXIT_FAILURE;
    }
    fcntl(pipe_fd, F_SETFL, O_NONBLOCK);

    json_tokener *toker = json_tokener_new();
    if (!toker) {
        printf("Error allocate json parser\n");
        return EXIT_FAILURE;
    }

    Player_t player = {0};
    if (!player_curl_init(&player)) {
        printf("Error init player\n");
        return EXIT_FAILURE;
    }

    uint8_t command_buffer[IPC_MAX_MESSAGE_LEN];
    memset(command_buffer, 0, IPC_MAX_MESSAGE_LEN);

    MediaSource_t media_source = {0};
    if (!make_url_source(&media_source)) {
        printf("Error allocate media source for player\n");
        player_curl_destroy(&player);
        return EXIT_FAILURE;
    }

    bool waiting_for_first_source = true;

    while (waiting_for_first_source) {
        printf("[INFO]: Waiting for media source\n");
        sleep(1);
    }

    while (true) {
        int read_status = read(pipe_fd, command_buffer, IPC_MAX_MESSAGE_LEN);
        if (read_status < 0 && errno != EAGAIN) {
            perror("Read error\n");
            return EXIT_FAILURE;
        }

        if (!player_process(&player)) {
            printf("Error while playing\n");
            return EXIT_FAILURE;
        }
    }

    json_tokener_free(main_tokener);
    player_curl_destroy(&player);
    return EXIT_SUCCESS;
}
