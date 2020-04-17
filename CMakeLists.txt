cmake_minimum_required(VERSION 2.8)
project(simple_ffmpeg_player)

option(USE_BM_CODEC "Use bitmain codec" OFF)
option(USE_SYSTEM_OPENCV "Use opencv display images" OFF)

if (NOT USE_SYSTEM_OPENCV)
    add_definitions(-DUSE_DUMPYUV_FILE=1)
else(USE_SYSTEM_OPENCV)
    find_package(OpenCV)
endif()

if (USE_BM_CODEC)
    add_definitions(-DUSE_BM_CODEC=1)
    set(BMNNSDK_TOP /home/yuan/bmnnsdk2-bm1684_v2.0.3)
    set(ffmpeg_inc ${BMNNSDK_TOP}/include/ffmpeg)
    set(ffmpeg_link_dirs ${BMNNSDK_TOP}/lib/ffmpeg/x86
            ${BMNNSDK_TOP}/lib/decode/x86)
    set(ffmpeg_link_libs avutil avformat avfilter avcodec swscale bmvideo)
else()
    set(ffmpeg_inc /usr/local/include)
    set(ffmpeg_link_dirs /usr/local/lib)
    set(ffmpeg_link_libs avutil avformat avfilter avcodec swscale)
endif()

include_directories(${ffmpeg_inc})
link_directories(${ffmpeg_link_dirs})

add_executable(simple_ffmpeg_player simple_ffmpeg_player.cpp)
target_link_libraries(simple_ffmpeg_player ${ffmpeg_link_libs} ${OpenCV_LIBS})
