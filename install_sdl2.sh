#!/bin/bash


#sudo apt-get install libfreetype6-dev libgl1-mesa-dev libgles2-mesa-dev libgbm-dev libudev-dev libasound2-dev liblzma-dev git build-essential
apt-get install \
libfreetype6-dev \
libgles2-mesa-dev \
libgbm-dev \
libudev-dev \
libasound2-dev \
liblzma-dev \
git \
build-essential \
gir1.2-ibus-1.0 \
libdbus-1-dev \
libegl1-mesa-dev \
libibus-1.0-5 \
libibus-1.0-dev \
libice-dev \
libsm-dev \
libsndio-dev \
libwayland-bin \
libwayland-dev \
libxi-dev \
libxinerama-dev \
libxkbcommon-dev \
libxrandr-dev \
libxss-dev \
libxt-dev \
libxv-dev \
x11proto-randr-dev \
x11proto-scrnsaver-dev \
x11proto-video-dev \
x11proto-xinerama-dev

cd ~
wget https://libsdl.org/release/SDL2-2.0.9.tar.gz
tar zxvf SDL2-2.0.9.tar.gz
cd SDL2-2.0.9
./configure --enable-video-kmsdrm --disable-video-opengl --disable-video-x11 --disable-video-rpi &&
make -j$(nproc) &&
make install
ldconfig