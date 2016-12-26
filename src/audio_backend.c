#include "audio_backend.h"
#include <errno.h>
#include <string.h>
#include "logger.h"

#include "pipe_backend.h"
#if ALSA
#include "alsa_backend.h"
#endif
#if PULSEAUDIO
#include "pulseaudio_backend.h"
#endif
#if JACK
#include "jack_backend.h"
#endif

struct backend_list_item_t
{
    char                    name[AUDIO_BACKEND_NAME_SIZE];
    audio_backend_init_f    init_function;
};

static struct backend_list_item_t const backend_list[] = 
{
    #if ALSA
    { ALSA_BACKEND_NAME, alsa_backend_init },
    #endif
    #if PULSEAUDIO
    { PULSEAUDIO_BACKEND_NAME, pulseaudio_backend_init },
    #endif
    #if JACK
    { JACK_BACKEND_NAME, jack_backend_init },
    #endif
    { PIPE_BACKEND_NAME, pipe_backend_init }
};

int audio_backend_get_by_name(char const* name, audio_backend_handle_t* backend)
{
    size_t index;

    for (index = 0; index != sizeof(backend_list) / sizeof(struct backend_list_item_t); ++index)
    {
        if (!strcmp(name, backend_list[index].name))
        {
            logger_log(LOG_DEBUG, "audio_backend_get_by_name: found backend %s", name);
            return backend_list[index].init_function(backend);
        }
    }

    logger_log(LOG_ERROR, "audio_backend_get_by_name: no backend found with name %s", name);

    return -EINVAL;
}
