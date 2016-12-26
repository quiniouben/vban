#ifndef __AUDIO_BACKEND_H__
#define __AUDIO_BACKEND_H__

#include <stdlib.h>
#include "vban.h"

#define AUDIO_BACKEND_NAME_SIZE     32

struct audio_backend_t;
typedef struct audio_backend_t* audio_backend_handle_t;

typedef int (*audio_backend_init_f) (audio_backend_handle_t* handle);
typedef int (*audio_backend_is_fmt_supported_f) (audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate);
typedef int (*audio_backend_open_f) (audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size);
typedef int (*audio_backend_close_f) (audio_backend_handle_t handle);
typedef int (*audio_backend_write_f) (audio_backend_handle_t handle, char const* data, size_t nb_sample);

struct audio_backend_t
{
    audio_backend_is_fmt_supported_f    is_fmt_supported;
    audio_backend_open_f                open;
    audio_backend_close_f               close;
    audio_backend_write_f               write;
};

int audio_backend_get_by_name(char const* name, audio_backend_handle_t* backend);

/*XXX provide helper text function */

#endif /*__AUDIO_BACKEND_H__*/

