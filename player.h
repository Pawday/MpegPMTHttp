#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum
{
    PLYR_IDLE,
    PLYR_PLAYING,
    PLYR_PAUSED,
    PLYR_ERROR
} PlayerState;

typedef struct
{
    void *impl_data;
    size_t impl_data_size;
} MediaSource_t;

typedef struct
{
    void *impl_data;
    size_t impl_data_size;
} Player_t;

bool player_set_source(Player_t *plyr, MediaSource_t *new_source);
bool player_start(Player_t *plyr);
bool player_process(Player_t *plyr);

PlayerState player_get_state(Player_t *plyr);

