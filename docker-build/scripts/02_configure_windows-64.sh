cd "${SPRING_DIR}"

mkdir -p "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"/bin-dir

if [ -d ./mingwlibs ]; then
	WORKDIR=$(pwd)/mingwlibs
fi
if [ -d ./mingwlibs64 ]; then
	WORKDIR=$(pwd)/mingwlibs64
fi

LIBDIR=$WORKDIR/dll
INCLUDEDIR=$WORKDIR/include

cmake \
	-DCMAKE_TOOLCHAIN_FILE="/scripts/${PLATFORM}.cmake" \
	-DMARCH_FLAG="${MYARCHTUNE}" \
	-DCMAKE_CXX_FLAGS="${MYCFLAGS}" \
	-DCMAKE_C_FLAGS="${MYCFLAGS}" \
	-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="${MYRWDIFLAGS}" \
	-DCMAKE_C_FLAGS_RELWITHDEBINFO="${MYRWDIFLAGS}" \
	-DCMAKE_BUILD_TYPE=RELWITHDEBINFO \
	-DAI_TYPES=NATIVE \
	-B "${BUILD_DIR}" \
	.
