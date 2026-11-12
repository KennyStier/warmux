# Native build for Android: SDL2 (shared, loaded by SDLActivity) and the
# game as libmain.so with the other libraries linked in statically.
# The libraries are downloaded into deps/ by packaging/android/prepare.sh.
cmake_minimum_required(VERSION 3.22)
project(warmux_android C CXX)

set(DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps)
get_filename_component(TOP ${CMAKE_CURRENT_SOURCE_DIR}/../../../.. ABSOLUTE)

macro(opt name value)
  set(${name} ${value} CACHE BOOL "" FORCE)
endmacro()

# --- SDL2 ---------------------------------------------------------------------
opt(SDL_SHARED ON)
opt(SDL_STATIC OFF)
opt(SDL_TEST OFF)
add_subdirectory(${DEPS}/SDL2 SDL2 EXCLUDE_FROM_ALL)
# The static add-on libraries ask for SDL2::SDL2-static: give them the
# shared SDL2, which SDLActivity loads anyway
add_library(sdl2_for_static INTERFACE)
target_link_libraries(sdl2_for_static INTERFACE SDL2::SDL2)
add_library(SDL2::SDL2-static ALIAS sdl2_for_static)

# Everything below is static, linked into libmain.so
opt(BUILD_SHARED_LIBS OFF)

opt(SDL2IMAGE_INSTALL OFF)
opt(SDL2IMAGE_SAMPLES OFF)
opt(SDL2IMAGE_VENDORED OFF)
opt(SDL2IMAGE_BACKEND_STB ON)   # PNG and JPEG via stb_image, no extra libraries
opt(SDL2IMAGE_AVIF OFF)
opt(SDL2IMAGE_JXL OFF)
opt(SDL2IMAGE_TIF OFF)
opt(SDL2IMAGE_WEBP OFF)
add_subdirectory(${DEPS}/SDL2_image SDL2_image EXCLUDE_FROM_ALL)

opt(SDL2MIXER_INSTALL OFF)
opt(SDL2MIXER_SAMPLES OFF)
opt(SDL2MIXER_VENDORED OFF)
opt(SDL2MIXER_DEPS_SHARED OFF)
opt(SDL2MIXER_VORBIS_STB ON)    # Ogg Vorbis via stb_vorbis (the game's music)
opt(SDL2MIXER_OPUS OFF)
opt(SDL2MIXER_FLAC OFF)
opt(SDL2MIXER_MOD OFF)
opt(SDL2MIXER_MIDI OFF)
opt(SDL2MIXER_MP3 OFF)
opt(SDL2MIXER_WAVPACK OFF)
opt(SDL2MIXER_GME OFF)
add_subdirectory(${DEPS}/SDL2_mixer SDL2_mixer EXCLUDE_FROM_ALL)

opt(SDL2TTF_INSTALL OFF)
opt(SDL2TTF_SAMPLES OFF)
opt(SDL2TTF_VENDORED ON)        # bundled FreeType
opt(SDL2TTF_HARFBUZZ OFF)
add_subdirectory(${DEPS}/SDL2_ttf SDL2_ttf EXCLUDE_FROM_ALL)

opt(SDL2NET_INSTALL OFF)
opt(SDL2NET_SAMPLES OFF)
add_subdirectory(${DEPS}/SDL2_net SDL2_net EXCLUDE_FROM_ALL)

# SDL2_gfx has no CMake build
file(GLOB GFX_SOURCES ${DEPS}/SDL2_gfx/SDL2_*.c)
add_library(SDL2_gfx STATIC ${GFX_SOURCES})
target_include_directories(SDL2_gfx PUBLIC ${DEPS}/SDL2_gfx)
target_link_libraries(SDL2_gfx PUBLIC SDL2::SDL2)

# libpng (screenshots) on the NDK's zlib
opt(PNG_SHARED OFF)
opt(PNG_STATIC ON)
opt(PNG_TESTS OFF)
opt(PNG_TOOLS OFF)
opt(SKIP_INSTALL_ALL ON)
add_subdirectory(${DEPS}/libpng libpng EXCLUDE_FROM_ALL)

# libxml2 (all game data is XML)
opt(LIBXML2_WITH_ICONV OFF)
opt(LIBXML2_WITH_ICU OFF)
opt(LIBXML2_WITH_LZMA OFF)
opt(LIBXML2_WITH_ZLIB OFF)
opt(LIBXML2_WITH_PYTHON OFF)
opt(LIBXML2_WITH_PROGRAMS OFF)
opt(LIBXML2_WITH_TESTS OFF)
opt(LIBXML2_WITH_HTTP OFF)
opt(LIBXML2_WITH_MODULES OFF)
add_subdirectory(${DEPS}/libxml2 libxml2 EXCLUDE_FROM_ALL)

# --- Warmux -------------------------------------------------------------------
# Source lists come from the automake files, so they stay in sync
function(automake_sources out makefile_am dir)
  # Re-run CMake when a source list changes
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${makefile_am})
  file(READ ${makefile_am} text)
  string(REGEX MATCHALL "[A-Za-z0-9_./-]+\\.cpp" files "${text}")
  set(result "")
  foreach(f ${files})
    if(NOT f MATCHES "^maemo/")
      list(APPEND result ${dir}/${f})
    endif()
  endforeach()
  set(${out} ${result} PARENT_SCOPE)
endfunction()

automake_sources(GAME_SOURCES ${TOP}/src/Makefile.am ${TOP}/src)
automake_sources(LIBWARMUX_SOURCES ${TOP}/lib/warmux/Makefile.am ${TOP}/lib/warmux)
automake_sources(FIXEDPOINT_SOURCES ${TOP}/lib/fixedpoint/Makefile.am ${TOP}/lib/fixedpoint)

set(WARMUX_INDEX_SERVER "")
set(WARMUX_META_URL "")
configure_file(${TOP}/lib/warmux/include/WARMUX_net_defaults.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/WARMUX_net_defaults.h @ONLY)

add_library(main SHARED
  ${GAME_SOURCES} ${LIBWARMUX_SOURCES} ${FIXEDPOINT_SOURCES}
  ${TOP}/src/android_data.cpp)
target_include_directories(main PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${TOP}/src ${TOP}/lib/warmux/include ${TOP}/lib/fixedpoint)
target_compile_definitions(main PRIVATE
  INSTALL_DATADIR="data/" INSTALL_LOCALEDIR="locale/"
  FONT_FILE="data/font/Ubuntu-R.ttf")
target_compile_options(main PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -fno-exceptions>
  -Wno-deprecated-declarations)
target_link_libraries(main PRIVATE
  SDL2::SDL2main SDL2::SDL2 SDL2_image::SDL2_image-static
  SDL2_mixer::SDL2_mixer-static SDL2_ttf::SDL2_ttf-static
  SDL2_net::SDL2_net-static SDL2_gfx png_static LibXml2::LibXml2
  log android z)
