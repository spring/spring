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
wget -N http://www.springlobby.info/installer/sl-installer-files.zip
wget -N http://www.springlobby.info/installer/springlobby.exe
cd ..

cd Springlobby

unzip ../downloads/sl-installer-files.zip -d SLArchive

mkdir SettingsDlls
mv SLArchive/mingwm10.dll SettingsDlls
mv SLArchive/wxbase28u_gcc_custom.dll SettingsDlls
mv SLArchive/wxbase28u_net_gcc_custom.dll SettingsDlls
mv SLArchive/wxmsw28u_adv_gcc_custom.dll SettingsDlls
mv SLArchive/wxmsw28u_core_gcc_custom.dll SettingsDlls
