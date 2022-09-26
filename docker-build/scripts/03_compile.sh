cd "${BUILD_DIR}"

echo "CCACHE_DEBUG=${CCACHE_DEBUG}"

make -j$(nproc) all
