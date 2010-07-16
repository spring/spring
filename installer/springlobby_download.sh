#! /bin/bash

# Quit on error.
set -e

if [ ! -e installer ]; then
  echo "Error: This script needs to be run from the root directory of the archive"
  exit 1
fi

cd installer
rm -rf Springlobby

mkdir -p Springlobby

### Download archive (if not already done)
mkdir -p downloads
cd downloads
wget -N http://www.springlobby.info/windows/latest.zip
cd ..

cd Springlobby

unzip ../downloads/latest.zip -d SLArchive

mkdir SettingsDlls
mv SLArchive/*.dll SettingsDlls
mv SLArchive/springsettings.exe SettingsDlls

### This version of OpenAL breaks Spring ...
# not used anymore since May 2010
rm -f SettingsDlls/OpenAL32.dll
