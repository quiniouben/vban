#ifndef __AUDIO_BACKEND_H__
#define __AUDIO_BACKEND_H__

#include <stdlib.h>
#include "vban/vban.h"
#include "common/audio.h"

struct audio_backend_t;
typedef struct audio_backend_t* audio_backend_handle_t;

typedef int (*audio_backend_init_f) (audio_backend_handle_t* handle);
typedef int (*audio_backend_open_f)     (audio_backend_handle_t handle, char const* output_name, enum audio_direction direction, size_t buffer_size, struct stream_config_t const* config);
typedef int (*audio_backend_close_f)    (audio_backend_handle_t handle);
typedef int (*audio_backend_write_f)    (audio_backend_handle_t handle, char const* data, size_t size);
typedef int (*audio_backend_read_f)     (audio_backend_handle_t handle, char* data, size_t size);

struct audio_backend_t
{
    audio_backend_open_f                open;
    audio_backend_close_f               close;
    audio_backend_write_f               write;
    audio_backend_read_f                read;
};

int audio_backend_get_by_name(char const* name, audio_backend_handle_t* backend);
char const* audio_backend_get_help();

#endif /*__AUDIO_BACKEND_H__*/

