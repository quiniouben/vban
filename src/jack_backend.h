#ifndef __JACK_BACKEND_H__
#define __JACK_BACKEND_H__

#include "audio_backend.h"

#define JACK_BACKEND_NAME   "jack"
int jack_backend_init(audio_backend_handle_t* handle);

#endif /*__JACK_BACKEND_H__*/

