cd "${BUILD_DIR}"

tag_name_short="{${BRANCH_NAME}\}"$(git describe --abbrev=7)
tag_name="{${BRANCH_NAME}}""$(git describe --abbrev=7)_${PLATFORM}"
bin_name=spring_bar_$tag_name-minimal-portable.7z
dbg_name=spring_bar_$tag_name-minimal-symbols.tgz

cd ./bin-dir
7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on ../$bin_name ./* -xr\!*.dbg
# export github output variables
echo "::set-output name=bin_name::${bin_name}"


if [ "${STRIP_SYMBOLS}" == "1" ]; then
    cd "${BUILD_DIR}"
    touch empty.dbg
    DEBUGFILES=$(find ./ -name '*.dbg')
    tar cvfz $dbg_name ${DEBUGFILES}
    echo "::set-output name=dbg_name::${dbg_name}"
fi
