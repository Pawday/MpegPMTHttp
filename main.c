#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <json-c/json.h>

#include "curl_media_source.h"
#include "main.h"
#include "mpegts/packet_inplace_parser.h"
#include "mpegts/pmt_builder.h"
#include "mpegts/pmt_dumper.h"
#include "player.h"

static json_tokener *main_json_tokener;

void print_string_escaped(char *string, size_t len)
{
    for (size_t char_index = 0; char_index < len; char_index++) {
        bool replace_char = false;

        char next_char = string[char_index];

        if (next_char == 0) {
            break;
        }

        if (!replace_char && next_char == '\n') {
            replace_char = true;
        }

        if (!replace_char) {
            putc(next_char, stdout);
            continue;
        }
        putc('?', stdout);
    }
}

bool parse_play_command(uint8_t *json_str, size_t len, PlayCommand *output)
{
    if (len > INT32_MAX) {
        return false;
    }

    json_tokener_reset(main_json_tokener);
    json_object *obj = json_tokener_parse_ex(main_json_tokener, (char *)json_str, len);
    if (!obj) {
        printf("[WARNING]: Failed to parse json from input cmd: \"");
        print_string_escaped((char *)json_str, len);
        puts("\"");
        return false;
    }

    json_object *cmd = NULL;
    if (!json_object_object_get_ex(obj, "cmd", &cmd)) {
        goto exit_parse_error;
    }
    const char *cmd_string = json_object_get_string(cmd);
    if (cmd_string == NULL) {
        goto exit_parse_error;
    }

    if (strcmp("play", cmd_string) != 0) {
        printf("[WARNING]: unrecognized command: \"");
        print_string_escaped((char *)cmd_string, IPC_MAX_MESSAGE_LEN);
        puts("\"");
        goto exit_parse_error;
    }

    json_object *url = NULL;
    if (!json_object_object_get_ex(obj, "url", &url)) {
        printf("[WARNING]: field \"url\" not found:\n");
        goto exit_parse_error;
    }

    int url_string_len = json_object_get_string_len(url);
    if (url_string_len > URL_MAX_LEN) {
        printf("[WARNING]: field \"url\" is too big\n");
        goto exit_parse_error;
    }

    const char *url_string = json_object_get_string(url);
    if (url_string == NULL) {
        goto exit_parse_error;
    }

    memset(output->url, 0, sizeof(output->url));
    memcpy(output->url, url_string, url_string_len);

    bool parse_status = true;
    goto exit_ok;
exit_parse_error:
    parse_status = false;
exit_ok:
    json_object_put(obj);
    return parse_status;
}

size_t build_string_from(int fd, uint8_t *buff, size_t buff_cap, const size_t init_write_offset)
{
    size_t bytes_readen_so_far = init_write_offset;
    int read_status;
    do {
        read_status = read(fd + bytes_readen_so_far, buff, buff_cap - bytes_readen_so_far);
        return bytes_readen_so_far;
    } while (read_status > 0);

    return bytes_readen_so_far;
}

int main(void)
{
    curl_global_init(0);

    if (mkfifo(PIPE_NAME, 0777) != 0 && errno != EEXIST) {
        perror("[ERROR]: making pipe");
        return EXIT_FAILURE;
    }

    int pipe_fd = open(PIPE_NAME, O_RDWR | O_CREAT, 0777);
    if (0 > pipe_fd) {
        perror("[ERROR]: open pipe");
        return EXIT_FAILURE;
    }
    fcntl(pipe_fd, F_SETFL, O_NONBLOCK);

    json_tokener *toker = json_tokener_new();
    if (!toker) {
        printf("[ERROR]: allocate json parser\n");
        goto exit_fail;
    }
    main_json_tokener = toker;

    Player_t player = {0};
    if (!player_init(&player)) {
        printf("[ERROR]: init player\n");
        goto exit_fail;
    }

    MediaSource_t curl_media_source = {0};
    if (!media_source_curl_init(&curl_media_source)) {
        printf("[ERROR] create curl media source\n");
        goto exit_fail;
    }

    uint8_t command_buffer[IPC_MAX_MESSAGE_LEN];
    memset(command_buffer, 0, IPC_MAX_MESSAGE_LEN);

    PlayCommand play_cmd = {0};

    printf("[INFO]: Waiting for commands\n");

    bool player_recvd_first_source = false;

    while (true) {

        if (!player_process(&player) && player_recvd_first_source) {
            printf("[ERROR]: while playing\n");
            goto exit_fail;
        }

        int readen_or_err = read(pipe_fd, command_buffer, IPC_MAX_MESSAGE_LEN);
        if (readen_or_err < 0 && errno != EAGAIN) {
            perror("[ERROR]: Read from pipe\n");
            goto exit_fail;
        }
        if (readen_or_err > 0) {
            size_t pipe_input_len =
                build_string_from(pipe_fd, command_buffer, sizeof(command_buffer), readen_or_err);

            if (!parse_play_command(command_buffer, pipe_input_len, &play_cmd)) {
                continue;
            }

            CURLMediaSource_t *curl_source_ptr;
            bool media_source_cast_status =
                media_source_curl_from_raw(&curl_media_source, &curl_source_ptr);
            assert(media_source_cast_status);
            if (!media_source_cast_status) {
                printf("[ERROR] cast raw source to curl source\n");
                goto exit_fail;
            }

            if (!media_source_curl_try_set_url(curl_source_ptr, play_cmd.url)) {
                printf("[WARNING]: Unsuported url: %s\n", play_cmd.url);
                continue;
            }

            if (player_recvd_first_source && !player_stop(&player)) {
                printf("[ERROR] stopping player\n");
                goto exit_fail;
            }

            if (!player_replace_media_source(&player, &curl_media_source)) {
                printf("[ERROR] replacing url\n");
                goto exit_fail;
            }

            player_recvd_first_source = true;

            printf("[INFO]: New url: %s\n", play_cmd.url);

            if (!player_start(&player)) {
                printf("[ERROR]: player start\n");
                goto exit_fail;
            }
        }
    }

    int exit_status = EXIT_SUCCESS;
    goto exit_success;

exit_fail:
    exit_status = EXIT_FAILURE;
exit_success:
    media_source_curl_destroy(&curl_media_source);
    json_tokener_free(main_json_tokener);
    player_destroy(&player);
    curl_global_cleanup();
    return exit_status;
}
