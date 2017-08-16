#include "audio_backend.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "common/logger.h"

#include "pipe_backend.h"
#include "file_backend.h"
#if ALSA
#include "alsa_backend.h"
#endif
#if PULSEAUDIO
#include "pulseaudio_backend.h"
#endif
#if JACK
#include "jack_backend.h"
#endif

#define HELP_TEXT_LEN   2048

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
    { PIPE_BACKEND_NAME, pipe_backend_init },
    { FILE_BACKEND_NAME, file_backend_init }
};

int audio_backend_get_by_name(char const* name, audio_backend_handle_t* backend)
{
    size_t index;

    if (name[0] == '\0')
    {
        logger_log(LOG_INFO, "%s: taking default backend %s", __func__, backend_list[0].name);
        return backend_list[0].init_function(backend);
    }

    for (index = 0; index != sizeof(backend_list) / sizeof(struct backend_list_item_t); ++index)
    {
        if (!strcmp(name, backend_list[index].name))
        {
            logger_log(LOG_INFO, "%s: found backend %s", __func__, name);
            return backend_list[index].init_function(backend);
        }
    }

    logger_log(LOG_ERROR, "%s: no backend found with name %s", __func__, name);

    return -EINVAL;
}

char const* audio_backend_get_help()
{
    static char help_text[HELP_TEXT_LEN];
    size_t index = 0;
    size_t offset = 0;

    offset += snprintf(help_text + offset, HELP_TEXT_LEN - offset, "Available audio backends are: ");
    for (index = 0; index < sizeof(backend_list) / sizeof(struct backend_list_item_t); ++index)
    {
        offset += snprintf(help_text + offset, HELP_TEXT_LEN - offset, "%s ", backend_list[index].name);
    }

    offset += snprintf(help_text + offset, HELP_TEXT_LEN - offset, ". default is %s.", backend_list[0].name);
    
    return help_text;
}

