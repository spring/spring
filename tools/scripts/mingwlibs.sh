#!/bin/sh
# Author: Tobi Vollebregt
#
# Simple helper script for automatic crosscompilation of spring using mingw.

[ -d mingwlibs ] && echo "up to date" && exit 0

url="http://spring.clan-sy.com/dl/spring-mingwlibs.exe"

echo "downloading mingwlibs"
wget -q -nc "$url"
echo "extracting mingwlibs"
7z x -y "spring-mingwlibs.exe"
echo "up to date"
