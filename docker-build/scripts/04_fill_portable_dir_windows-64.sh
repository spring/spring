# do not break on error here
set +e

cd "${BUILD_DIR}"

rm -rf ./bin-dir/*
if [ -d ../mingwlibs64 ]; then
cp ../mingwlibs64/dll/* ./bin-dir/
fi
if [ -d ../mingwlibs ]; then
cp ../mingwlibs/dll/* ./bin-dir/
fi
cp ./*.exe ./bin-dir/
cp ../rts/lib/luasocket/src/socket.lua ./bin-dir/
cp ./tools/pr-downloader/src/pr-downloader.exe ./bin-dir/
touch ./bin-dir/springsettings.cfg
truncate -s 0 ./bin-dir/springsettings.cfg
cp ./unitsync.dll ./bin-dir/
cp ../cont/luaui.lua ./bin-dir/luaui.lua
cp -R ../cont/LuaUI ./bin-dir/LuaUI
mkdir ./bin-dir/games
mkdir ./bin-dir/maps
cp -R ../cont/fonts ./bin-dir/fonts
cp -R ../cont/examples ./bin-dir/examples
mkdir ./bin-dir/doc
cp ../AUTHORS ./bin-dir/doc/AUTHORS
cp ../COPYING ./bin-dir/doc/COPYING
cp ../FAQ ./bin-dir/doc/FAQ
cp ../gpl-2.0.txt ./bin-dir/doc/gpl-2.0.txt
cp ../gpl-3.0.txt ./bin-dir/doc/gpl-3.0.txt
cp ../LICENSE ./bin-dir/doc/LICENSE
cp ../README.markdown ./bin-dir/doc/README.markdown
cp ../THANKS ./bin-dir/doc/THANKS
cp ../cont/ctrlpanel.txt ./bin-dir/ctrlpanel.txt
cp ../cont/cmdcolors.txt ./bin-dir/cmdcolors.txt
cp -R ./base ./bin-dir/base
mkdir -p ./bin-dir/AI/Interfaces/C/0.1/
cp -R ./AI/Interfaces/C/data/* ./bin-dir/AI/Interfaces/C/0.1/
cp -R ../AI/Interfaces/C/data/* ./bin-dir/AI/Interfaces/C/0.1/
mkdir -p ./bin-dir/AI/Skirmish/NullAI/0.1
cp -R ./AI/Skirmish/NullAI/data/* ./bin-dir/AI/Skirmish/NullAI/0.1/
cp -R ../AI/Skirmish/NullAI/data/* ./bin-dir/AI/Skirmish/NullAI/0.1/
mkdir -p ./bin-dir/AI/Skirmish/CircuitAI/stable/
cp -R ./AI/Skirmish/CircuitAI/data/* ./bin-dir/AI/Skirmish/CircuitAI/stable/
cp -R ../AI/Skirmish/CircuitAI/data/* ./bin-dir/AI/Skirmish/CircuitAI/stable/
mkdir -p ./bin-dir/AI/Skirmish/BARb/stable/
cp -R ./AI/Skirmish/BARb/data/* ./bin-dir/AI/Skirmish/BARb/stable/
cp -R ../AI/Skirmish/BARb/data/* ./bin-dir/AI/Skirmish/BARb/stable/

set -e