#! /bin/bash

if [ ! -e installer ]; then
  echo "Error: This script needs to be run from the root directory of the archive"
  exit 1
fi

cd installer
mkdir -p external
rm -rf TASClient

mkdir -p TASClient/SaveFiles
cd TASClient

wget http://tasclient.it-l.eu/Installer_Archive.7z
7z x Installer_Archive.7z -oSaveFiles
rm Installer_Archive.7z

mv SaveFiles/Lobby/var/groups.ini .
