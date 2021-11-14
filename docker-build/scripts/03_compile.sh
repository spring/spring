cd "${BUILD_DIR}"

if [ -z "${ENABLE_CCACHE}" ]; then
    echo "Disabling ccache"
    export CCACHE_DISABLE=1
else
    echo "Using ccache in directory: ${CCACHE_DIR}"
fi

CLEANLIST_LIN=$(find -maxdepth 1 -name '*.so')" "$(find -maxdepth 1 -name 'spring*' -executable)" "$(find -maxdepth 1 -name 'pr-downloader')" "$(find AI/Skirmish -name libSkirmishAI.so)" "$(find AI/Interfaces -name libAIInterface.so)
CLEANLIST_WIN=$(find -maxdepth 1 -name '*.dll')" "$(find -maxdepth 1 -name '*.exe')" "$(find AI/Skirmish -name SkirmishAI.dll)" "$(find AI/Interfaces -name AIInterface.dll)" "$(find -name pr-downloader_shared.dll)
CLEANLIST_VAR=$(find -name 'GameVersion.cpp.o')
CLEANLIST=$CLEANLIST_LIN" "$CLEANLIST_WIN" "$CLEANLIST_VAR
rm -f $CLEANLIST VERSION

make -j$(nproc) all
