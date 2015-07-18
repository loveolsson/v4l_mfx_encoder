#/bin/bash
g++ -o hwencoder hwencoder.c libav.c `pkg-config --cflags --libs libavformat libswscale libavdevice libavcodec libavutil` -Wall `sdl-config --cflags --libs` -D__STDC_CONSTANT_MACROS -std=c++11
