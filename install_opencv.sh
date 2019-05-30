#sudo apt-get remove libopencv*
#sudo apt-get autoremove
#sudo apt-get update
#sudo apt-get upgrade

apt-get -y install build-essential
apt-get -y install cmake
apt-get -y install cmake-curses-gui
apt-get -y install pkg-config
apt-get -y install unzip
apt-get -y install libjpeg-dev
apt-get -y install libtiff5-dev
apt-get -y install libjasper-dev
apt-get -y install libpng-dev
apt-get -y install libavcodec-dev
apt-get -y install libavformat-dev
apt-get -y install libswscale-dev
apt-get -y install libeigen3-dev
apt-get -y install libxvidcore-dev
apt-get -y install libx264-dev
apt-get -y install libgtk2.0-dev
apt-get -y install libgtk-3-dev
apt-get -y install libcanberra-gtk*
apt-get -y install libv4l-dev
apt-get -y install v4l-utils
modprobe bcm2835-v4l2
apt-get -y install libatlas-base-dev
apt-get -y install gfortran
apt-get -y install libgtkglext1
apt-get -y install libgtkglext1-dev

wget https://github.com/opencv/opencv/archive/4.1.0.zip -O opencv_source.zip
wget https://github.com/opencv/opencv_contrib/archive/4.1.0.zip -O opencv_contrib.zip
unzip opencv_source.zip
unzip opencv_contrib.zip
cd opencv-4.1.0
mkdir build
cd build
cmake \
	-D CMAKE_BUILD_TYPE=RELEASE \
	-D CMAKE_INSTALL_PREFIX=/usr/local \
	-D BUILD_WITH_DEBUG_INFO=OFF \
	-D BUILD_DOCS=OFF \
	-D BUILD_EXAMPLES=OFF \
	-D BUILD_TESTS=OFF \
	-D BUILD_opencv_ts=OFF \
	-D BUILD_PERF_TESTS=OFF \
	-D INSTALL_C_EXAMPLES=OFF \
	-D INSTALL_PYTHON_EXAMPLES=OFF \
	-D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-4.1.0/modules \
	-D ENABLE_NEON=ON \
	-D WITH_LIBV4L=ON \
	-D WITH_OPENGL=ON \
	../

make -j3
make install
ldconfig
