#! /bin/bash

# Quit on error.
set -e

if [ ! -e installer ]; then
  echo "Error: This script needs to be run from the root directory of the archive"
  exit 1
fi

cd installer

### Download archive (if not already done)
mkdir -p downloads
cd downloads
wget -N http://tasclient.licho.eu/TASClientLatest.7z
cd ..

rm -rf TASClient

mkdir -p TASClient/SaveFiles
cd TASClient

7z x ../downloads/TASClientLatest.7z -oSaveFiles

# mv SaveFiles/Lobby/var/groups.ini .
