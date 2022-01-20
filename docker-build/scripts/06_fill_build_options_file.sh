cd "${BUILD_DIR}"

export FILENAME_BUILDOPTIONS="buildoptions_${PLATFORM}.txt"

rm -rf "${FILENAME_BUILDOPTIONS}" || true
touch "${FILENAME_BUILDOPTIONS}"

MYCXX_FLAGS=$(cat /spring/build/rts/builds/legacy/CMakeFiles/engine-legacy.dir/flags.make | grep CXX_FLAGS)

echo "MYARCHTUNE=$MYARCHTUNE" >> "${FILENAME_BUILDOPTIONS}"
echo "MYCFLAGS=$MYCFLAGS" >> "${FILENAME_BUILDOPTIONS}"
echo "MYRWDIFLAGS=$MYRWDIFLAGS" >> "${FILENAME_BUILDOPTIONS}"
echo "$MYCXX_FLAGS" >> "${FILENAME_BUILDOPTIONS}"
echo "PLATFORM=$PLATFORM" >> "${FILENAME_BUILDOPTIONS}"
echo "BRANCH=$BRANCH_NAME" >> "${FILENAME_BUILDOPTIONS}"
echo "DUMMY=$DUMMY" >> "${FILENAME_BUILDOPTIONS}"
echo "ENABLE_CCACHE=$ENABLE_CCACHE" >> "${FILENAME_BUILDOPTIONS}"
echo "STRIP_SYMBOLS=$STRIP_SYMBOLS" >> "${FILENAME_BUILDOPTIONS}"

echo "SPRING_DIR=$SPRING_DIR" >> "${FILENAME_BUILDOPTIONS}"
echo "BUILD_DIR=$BUILD_DIR" >> "${FILENAME_BUILDOPTIONS}"
echo "PUBLISH_DIR=$PUBLISH_DIR" >> "${FILENAME_BUILDOPTIONS}"