# do not break on error here
set +e

cd "${BUILD_DIR}"

rm -rf ./bin-dir/*
cp ./spring* ./bin-dir/
cp ../rts/lib/luasocket/src/socket.lua ./bin-dir/
cp ./tools/pr-downloader/src/pr-downloader ./bin-dir/
touch ./bin-dir/springsettings.cfg
truncate -s 0 ./bin-dir/springsettings.cfg
cp ./libunitsync.so ./bin-dir/
mkdir -p ./bin-dir/bin/
cp ./tools/mapcompile/mapcompile ./bin-dir/bin/
cp ../cont/luaui.lua ./bin-dir/luaui.lua
cp -R ../cont/LuaUI ./bin-dir/LuaUI
cp -R ../cont/fonts ./bin-dir/fonts
cp -R ../cont/examples ./bin-dir/examples
mkdir -p ./bin-dir/share/applications
cp -R ../cont/freedesktop/applications/* ./bin-dir/share/applications/
mkdir -p ./bin-dir/share/mime/packages
cp -R ../cont/freedesktop/mime/* ./bin-dir/share/mime/packages
mkdir -p ./bin-dir/share/pixmaps
cp -R ../cont/freedesktop/pixmaps/* ./bin-dir/share/pixmaps/
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