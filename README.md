# simple_ffmpeg_player
Simple player based Ffmpeg, it can play file, URLs(rtsp, rtmp, etc.)
This program support MacOS and Linux, it isn't tested on Windows.

1. compile with BMCODEC support.
   mkdir build
   cd build
   cmake -DUSE_BM_CODEC=1 ..
   make -j
2. When the flag USE_SYSTEM_OPENCV is ON, use OpenCV to show YUV in realtime.
   When the flag USE_SYSTEM_OPENCV is OFF, use File to store the YUV.
   To play YUV File use command line:
   ffplay -s WIDTHxHEIGHT -pix_fmt nv12 output.yuv

3. ./compile.sh or ./compile.sh bmcodec
