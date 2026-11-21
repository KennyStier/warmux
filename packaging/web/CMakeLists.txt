# Web build with Emscripten: the game as warmux.wasm with its data in
# warmux.data, loaded by warmux.html. SDL2 and its image, mixer, ttf and net
# libraries, libpng and zlib come from Emscripten's ports; SDL2_gfx and
# libxml2 are downloaded into deps/ by packaging/web/prepare.sh.
#   emcmake cmake -S packaging/web -B packaging/web/build -DCMAKE_BUILD_TYPE=Release
#   cmake --build packaging/web/build -j
cmake_minimum_required(VERSION 3.22)
project(warmux_web C CXX)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "Configure with emcmake (Emscripten)")
endif()

set(DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps)
get_filename_component(TOP ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)

macro(opt name value)
  set(${name} ${value} CACHE BOOL "" FORCE)
endmacro()

# --- Emscripten ports, for compiling and linking ------------------------------
set(PORTS
  "SHELL:-sUSE_SDL=2"
  "SHELL:-sUSE_SDL_IMAGE=2" "SHELL:-sSDL2_IMAGE_FORMATS=[\"png\",\"jpg\"]"
  "SHELL:-sUSE_SDL_MIXER=2" "SHELL:-sSDL2_MIXER_FORMATS=[\"ogg\",\"wav\"]"
  "SHELL:-sUSE_SDL_TTF=2"
  "SHELL:-sUSE_SDL_NET=2"
  "SHELL:-sUSE_LIBPNG=1"
  "SHELL:-sUSE_ZLIB=1")
add_compile_options(${PORTS})
add_link_options(${PORTS})

opt(BUILD_SHARED_LIBS OFF)

# SDL2_gfx has no CMake build
file(GLOB GFX_SOURCES ${DEPS}/SDL2_gfx/SDL2_*.c)
add_library(SDL2_gfx STATIC ${GFX_SOURCES})
target_include_directories(SDL2_gfx PUBLIC ${DEPS}/SDL2_gfx)

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
opt(LIBXML2_WITH_THREADS OFF)
add_subdirectory(${DEPS}/libxml2 libxml2 EXCLUDE_FROM_ALL)

# --- Warmux -------------------------------------------------------------------
# Source lists come from the automake files, so they stay in sync
function(automake_sources out makefile_am dir)
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

# Testing builds (-DWARMUX_TESTING=1): see --enable-testing in configure.ac
if(NOT WARMUX_TESTING)
  set(WARMUX_TESTING 0)
endif()
if(WARMUX_TESTING)
  set(WARMUX_VERSION_SUFFIX "-testing")
  set(WARMUX_INDEX_SERVER "index.warmux.net:9998")
else()
  set(WARMUX_VERSION_SUFFIX "")
  set(WARMUX_INDEX_SERVER "index.warmux.net")
endif()
set(WARMUX_META_URL "")
configure_file(${TOP}/lib/warmux/include/WARMUX_net_defaults.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/WARMUX_net_defaults.h @ONLY)

add_executable(warmux ${GAME_SOURCES} ${LIBWARMUX_SOURCES} ${FIXEDPOINT_SOURCES}
  ${TOP}/src/web/web_data.cpp)

# The data: what the game needs to start goes in warmux.data (kept by the
# browser between visits); map images, battle music and some fonts are
# served from lazy/ and downloaded when first used (see split_data.sh)
add_custom_target(webdata
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/split_data.sh ${TOP}/data ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Splitting the game data"
  VERBATIM)
add_dependencies(warmux webdata)
set_target_properties(warmux PROPERTIES SUFFIX ".html")
target_include_directories(warmux PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${TOP}/src ${TOP}/lib/warmux/include ${TOP}/lib/fixedpoint)
# The data is packed at /data in the browser's virtual file system
target_compile_definitions(warmux PRIVATE
  INSTALL_DATADIR="/data/" INSTALL_LOCALEDIR="/locale/"
  FONT_FILE="/data/font/Ubuntu-R.ttf")
target_compile_options(warmux PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -fno-exceptions>
  -Wno-deprecated-declarations)
target_link_libraries(warmux PRIVATE SDL2_gfx LibXml2::LibXml2)

# The game's menus and battle each run their own loop: Asyncify lets them
# hand control back to the browser every frame without being rewritten
target_link_options(warmux PRIVATE
  "SHELL:-sASYNCIFY"
  "SHELL:-sASYNCIFY_STACK_SIZE=1048576"
  "SHELL:-sSTACK_SIZE=4MB"
  "SHELL:-sALLOW_MEMORY_GROWTH=1"
  "SHELL:-sINITIAL_MEMORY=256MB"
  "SHELL:-sEXIT_RUNTIME=1"
  "SHELL:-lidbfs.js"
  "SHELL:-sFORCE_FILESYSTEM=1"
  "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html"
  "SHELL:--preload-file ${CMAKE_CURRENT_BINARY_DIR}/core@/data"
  "SHELL:--use-preload-cache")
set_property(TARGET warmux APPEND PROPERTY LINK_DEPENDS
  ${TOP}/data ${CMAKE_CURRENT_BINARY_DIR}/core/lazy.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/shell.html)

# The page's wallpaper while the game loads: the menu's own background
add_custom_command(TARGET warmux POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${TOP}/data/menu/background.jpg
          ${CMAKE_CURRENT_BINARY_DIR}/background.jpg)
