#!/bin/sh

# used in .travis.yml

# travis-ci hack which enables the ipv6 loopback address
sudo ip addr add dev lo ::1

set -e

# sdl2
sudo add-apt-repository --yes ppa:bartbes/love-stable
# cmake
sudo add-apt-repository --yes ppa:george-edison55/precise-backports
# gcc 4.7
sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test
# boost
sudo add-apt-repository --yes ppa:boost-latest/ppa

sudo apt-get update -qq

