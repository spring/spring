#!/bin/sh

# used in .travis.yml

set -e

if [ "$CXX" = "g++" ]; then
	sudo apt-get install gcc-4.7 g++-4.7;
	sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.7 90;
	sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.7 90;
fi

sudo apt-get install libglew-dev libsdl2-dev libdevil-dev libopenal-dev libogg-dev \
  libvorbis-dev libfreetype6-dev p7zip-full libxcursor-dev libunwind7-dev

sudo apt-get install libboost-thread1.48-dev libboost-regex1.48-dev libboost-system1.48-dev \
  libboost-program-options1.48-dev libboost-signals1.48-dev libboost-chrono1.48-dev \
  libboost-filesystem1.48-dev libboost-test1.48-dev binutils-gold

