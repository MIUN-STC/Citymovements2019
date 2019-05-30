#!/bin/bash



g++ src/mqtt_track.cpp \
-Isrc \
-Wall \
-Wextra \
-Wundef \
-Wshadow \
-Wpointer-arith \
-Wcast-align \
-Wstrict-overflow=5 \
-Wwrite-strings \
-Wcast-qual \
-Wswitch-default \
-Wswitch-enum \
-Wconversion \
-Wunreachable-code \
-fsanitize=signed-integer-overflow \
-lm \
-I/usr/local/include/opencv4 \
-lopencv_core \
-lopencv_features2d \
-lopencv_imgproc \
-lopencv_video \
-obin/mqtt_track