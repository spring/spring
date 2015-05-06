#!/bin/sh

# used in .travis.yml

set -e

# sdl2
add-apt-repository ppa:bartbes/love-stable --yes
# gcc 4.7
if [ "$CXX" == "g++" ]; then
	add-apt-repository ppa:ubuntu-toolchain-r/test --yes
fi

apt-get update -qq

