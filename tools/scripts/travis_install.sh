#!/bin/sh

# used in .travis.yml

set -e

if [ "$CXX" = "g++" ]; then
	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
	sudo apt-get update
	sudo apt-get -y install gcc-7 g++-7
	sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-7
else
	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
	sudo apt-get update
	sudo apt-get -y install libstdc++-7-dev libgcc-7-dev
fi

sudo apt-get install -y libglew-dev libsdl2-dev libdevil-dev libopenal-dev libogg-dev \
  libvorbis-dev libfreetype6-dev p7zip-full libxcursor-dev libunwind8-dev libgles2-mesa-dev

sudo apt-get install -y binutils-gold cmake cmake-data

