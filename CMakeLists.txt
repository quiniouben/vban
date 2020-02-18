cmake_minimum_required(VERSION 3.5..3.10)

project(vban C)
message(STATUS "CMake version ${CMAKE_VERSION}")

# https://cmake.org/cmake/help/latest/prop_tgt/C_STANDARD.html
string(COMPARE EQUAL "${CMAKE_C_STANDARD}" "" no_cmake_c_standard_set)
if(no_cmake_c_standard_set)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)
    message(STATUS "Using default C standard ${CMAKE_C_STANDARD}")
else()
    message(STATUS "Using user specified C standard ${CMAKE_C_STANDARD}")
endif()
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

option(WITH_ALSA       "Build vban with ALSA support"       ON)
option(WITH_PULSEAUDIO "Build vban with PulseAudio support" ON)
option(WITH_JACK       "Build vban with JACK support"       ON)

#set(CMAKE_VERBOSE_MAKEFILE ON)

# add macro define 'DEBUG' to debug build
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

# enable various compiler warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -pedantic") # more warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsign-compare")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshadow")
# disable specific compiler warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-multichar") # also disables other unused warnings

if(WITH_ALSA)
    find_package(ALSA QUIET)
    if(ALSA_FOUND)
        message(STATUS "found dependency ALSA: ${ALSA_VERSION_STRING} '${ALSA_INCLUDE_DIRS}' '${ALSA_LIBRARIES}'")
    else()
        message(FATAL_ERROR "missing ALSA dependency. If you want to disable backend set WITH_ALSA=No")
    endif()
else()
    message(STATUS "building without ALSA backend")
endif()
if(WITH_PULSEAUDIO)
    find_package(PulseAudio QUIET)
    if(PulseAudio_FOUND)
        message(STATUS "found dependency PulseAudio: ${PULSEAUDIO_VERSION} '${PULSEAUDIO_INCLUDE_DIR}' '${PULSEAUDIO_LIBRARY}'")
    else()
        message(FATAL_ERROR "missing PulseAudio dependency. If you want to disable backend set WITH_PULSEAUDIO=No")
    endif()
else()
    message(STATUS "building without PulseAudio backend")
endif()
if(WITH_JACK)
    include(FindPkgConfig)
    pkg_search_module(JACK jack QUIET)
    if(JACK_FOUND)
        message(STATUS "found dependency JACK: ${JACK_VERSION} '${JACK_INCLUDEDIR}' '${JACK_LIBRARIES}'")
    else()
        message(STATUS "missing JACK dependency, disabling backend")
        message(FATAL_ERROR "missing JACK dependency. If you want to disable backend set WITH_JACK=No")
    endif()
else()
    message(STATUS "building without JACK backend")
endif()


# We want to compile source in src, so let's go there
add_subdirectory(src)
