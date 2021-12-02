cd "${INSTALL_DIR}"

if [ "${PLATFORM}" == "windows-64" ]; then
    EXECUTABLES=$(find -maxdepth 1 -name '*.dll')" "$(find -maxdepth 1 -name '*.exe')" "$(find AI/Skirmish -name SkirmishAI.dll)" "$(find AI/Interfaces -name AIInterface.dll)" "$(find -name pr-downloader_shared.dll)
elif [ "${PLATFORM}" == "linux-64" ]; then
    EXECUTABLES=$(find -maxdepth 1 -name '*.so')" "$(find -maxdepth 1 -name 'spring*' -executable)" "$(find -maxdepth 1 -name 'pr-downloader')" "$(find AI/Skirmish -name libSkirmishAI.so)" "$(find AI/Interfaces -name libAIInterface.so)
else
    echo "Unsupported platform: '${PLATFORM}'"
    exit 1
fi

for tostripfile in ${EXECUTABLES}; do
    if [ -f ${tostripfile} ]; then
        if ! objdump -h ${tostripfile} | grep -q .gnu_debuglink; then
            debugfile=$(dirname $tostripfile)/$(echo $(basename $tostripfile) | cut -f 1 -d '.').dbg
            echo "stripping ${tostripfile}, producing ${debugfile}"
            (objcopy --only-keep-debug ${tostripfile} ${debugfile} && \
                strip --strip-debug --strip-unneeded ${tostripfile} && \
                objcopy --add-gnu-debuglink=${debugfile} ${tostripfile})
        else
            echo "not stripping ${tostripfile}"
        fi
    fi
done
wait