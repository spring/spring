#!/bin/sh

# used in .travis.yml

set -e

sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get -y install gcc-7 g++-7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-7

sudo apt-get install -y libglew-dev libsdl2-dev libdevil-dev libopenal-dev libogg-dev \
  libvorbis-dev libfreetype6-dev p7zip-full libxcursor-dev libunwind8-dev libgles2-mesa-dev

sudo apt-get install -y libboost-thread1.55-dev libboost-regex1.55-dev libboost-system1.55-dev \
  libboost-program-options1.55-dev libboost-signals1.55-dev libboost-chrono1.55-dev \
  libboost-filesystem1.55-dev libboost-test1.55-dev binutils-gold cmake cmake-data

