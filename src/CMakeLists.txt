add_executable(vban_receptor
    receptor/main.c
    common/version.h
    common/audio.h
    common/audio.c
    common/packet.h
    common/packet.c
    common/backend/audio_backend.h
    common/backend/audio_backend.c
    common/backend/pipe_backend.c
    common/backend/pipe_backend.h
    common/backend/file_backend.c
    common/backend/file_backend.h
    common/socket.h
    common/socket.c
    common/stream.h
    common/stream.c
    vban/vban.h
    common/logger.h
    common/logger.c)

add_executable(vban_emitter
    emitter/main.c
    common/version.h
    common/audio.h
    common/audio.c
    common/packet.h
    common/packet.c
    common/backend/audio_backend.h
    common/backend/audio_backend.c
    common/backend/pipe_backend.c
    common/backend/pipe_backend.h
    common/backend/file_backend.c
    common/backend/file_backend.h
    common/socket.h
    common/socket.c
    common/stream.h
    common/stream.c
    vban/vban.h
    common/logger.h
    common/logger.c)

add_executable(vban_sendtext
    sendtext/main.c
    common/version.h
    common/socket.h
    common/socket.c
    vban/vban.h
    common/logger.h
    common/logger.c)
    
include(GNUInstallDirs)

foreach(exe vban_receptor vban_emitter vban_sendtext)
    target_include_directories(${exe} PRIVATE .)
    if(ALSA)
        target_compile_definition( ${exe} PRIVATE ALSA)
        target_include_directories(${exe} PRIVATE "${ALSA_INCLUDE_DIRS}")
        target_link_libraries(     ${exe} PRIVATE "${ALSA_LIBRARIES}")
    endif()
    if(PULSEAUDIO)
        target_compile_definition( ${exe} PRIVATE PULSEAUDIO)
        target_include_directories(${exe} PRIVATE "${PulSEAudio_INCLUDE_DIRS}")
        target_link_libraries(     ${exe} PRIVATE "${PulseAudio_LIBRARIES}")
    endif()
    if(JACK)
        target_compile_definition( ${exe} PRIVATE JACK)
        target_include_directories(${exe} PRIVATE "${JACK_INCLUDE_DIR}")
        target_link_libraries(     ${exe} PRIVATE "${JACK_LIBRARIES}")
    endif()
    
    install(TARGETS ${exe} DESTINATION "${CMAKE_INSTALL_BINDIR}")
endforeach()

if(ALSA)
    target_sources(vban_receptor PRIVATE
        common/backend/alsa_backend.h
        common/backend/alsa_backend.c)
    target_sources(vban_emitter PRIVATE
        common/backend/alsa_backend.h
        common/backend/alsa_backend.c)
endif()

if(PULSEAUDIO)
    target_sources(vban_receptor PRIVATE
        common/backend/pulseaudio_backend.h
        common/backend/pulseaudio_backend.c)
    target_sources(vban_emitter PRIVATE
        common/backend/pulseaudio_backend.h
        common/backend/pulseaudio_backend.c)
endif()

if(JACK)
    target_sources(vban_receptor PRIVATE
        common/backend/jack_backend.h
        common/backend/jack_backend.c)
    target_sources(vban_emitter PRIVATE
        common/backend/jack_backend.h
        common/backend/jack_backend.c)
endif()
