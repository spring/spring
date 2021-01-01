#!/bin/sh
set -e

WORKDIR=${PWD}
TMPDIR=${WORKDIR}/tmp
INCLUDEDIR=${WORKDIR}/include
LIBDIR=${WORKDIR}/lib
MAKE="make -j2"

rm -rf ${TMPDIR}
rm -rf ${INCLUDEDIR}
rm -rf ${LIBDIR}

mkdir -p ${TMPDIR}
mkdir -p ${INCLUDEDIR}
mkdir -p ${LIBDIR}


# zlib
cd ${TMPDIR}
wget http://www.zlib.net/zlib-1.2.11.tar.gz
tar xzf zlib-1.2.11.tar.gz
cd zlib-1.2.11
CFLAGS="-fPIC -DPIC -m32" LDFLAGS=-m32 ./configure --prefix ${WORKDIR}
${MAKE}
${MAKE} install



# libpng
cd ${TMPDIR}
wget http://prdownloads.sourceforge.net/libpng/libpng-1.6.34.tar.gz
tar xzf libpng-1.6.34.tar.gz
cd libpng-1.6.34
#./configure --enable-static --prefix ${WORKDIR}
${MAKE} -f scripts/makefile.linux CFLAGS="-fPIC -DPIC -m32" LDFLAGS=-m32 ZLIBLIB=${LIBDIR} ZLIBINC=${INCLUDEDIR} prefix=${WORKDIR} libpng.a
${MAKE} -f scripts/makefile.linux prefix=${WORKDIR} install

# libjpeg
cd ${TMPDIR}
wget http://www.ijg.org/files/jpegsrc.v9b.tar.gz
tar xzf jpegsrc.v9b.tar.gz
cd jpeg-9b
CFLAGS=-m32 LDFLAGS=-m32 ./configure --with-pic --prefix ${WORKDIR} --host=i686-linux-gnu
${MAKE}
${MAKE} install


# libtiff
cd ${TMPDIR}
wget http://download.osgeo.org/libtiff/tiff-4.0.8.tar.gz
tar xzf tiff-4.0.8.tar.gz
cd tiff-4.0.8
CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32 ./configure --with-pic --disable-lzma --disable-jbig --prefix ${WORKDIR} --host=i686-linux-gnu
${MAKE}
${MAKE} install


# libIL (DevIL)
cd ${TMPDIR}
git clone --depth 1 https://github.com/DentonW/DevIL
cd DevIL/DevIL
cmake . -DCMAKE_CXX_FLAGS="-fPIC -m32" -DBUILD_SHARED_LIBS=0 -DCMAKE_INSTALL_PREFIX=${WORKDIR} \
        -DPNG_PNG_INCLUDE_DIR=${INCLUDEDIR} -DPNG_LIBRARY_RELEASE=${LIBDIR}/libpng.a \
        -DJPEG_INCLUDE_DIR=${INCLUDEDIR} -DJPEG_LIBRARY=${LIBDIR}/libjpeg.a \
        -DTIFF_INCLUDE_DIR=${INCLUDEDIR} -DTIFF_LIBRARY_RELEASE=${LIBDIR}/libtiff.a \
        -DZLIB_INCLUDE_DIR=${INCLUDEDIR} -DZLIB_LIBRARY_RELEASE=${LIBDIR}/libz.a
${MAKE}
${MAKE} install

# libunwind
cd ${TMPDIR}
wget http://download.savannah.nongnu.org/releases/libunwind/libunwind-1.2.1.tar.gz
tar xzf libunwind-1.2.1.tar.gz
cd libunwind-1.2.1
CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32 ./configure --with-pic --disable-minidebuginfo --prefix ${WORKDIR} --host=i686-linux-gnu
${MAKE}
${MAKE} install

# glew
cd ${TMPDIR}
wget https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0.tgz
tar xzf glew-2.1.0.tgz
cd glew-2.1.0
${MAKE} GLEW_PREFIX=${WORKDIR} GLEW_DEST=${WORKDIR} LIBDIR=${LIBDIR} CFLAGS.EXTRA=-m32 LDFLAGS.EXTRA=-m32
${MAKE} GLEW_PREFIX=${WORKDIR} GLEW_DEST=${WORKDIR} LIBDIR=${LIBDIR} install
