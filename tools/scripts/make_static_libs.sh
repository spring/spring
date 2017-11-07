#!/bin/sh
set -e

WORKDIR=${PWD}
TMPDIR=${WORKDIR}/tmp
INCLUDEDIR=${WORKDIR}/include
LIBDIR=${WORKDIR}/lib
MAKE="make -j3"

rm -rf ${TMPDIR}

mkdir ${TMPDIR}
mkdir -p ${INCLUDEDIR}
mkdir -p ${LIBDIR}


# libpng
cd ${TMPDIR}
wget http://prdownloads.sourceforge.net/libpng/libpng-1.6.34.tar.gz
tar xzf libpng-1.6.34.tar.gz
cd libpng-1.6.34
./configure --enable-static --with-pic
${MAKE}
cp -f *.h ${INCLUDEDIR}
cp -f .libs/libpng16.a ${LIBDIR}/libpng.a


# zlib
cd ${TMPDIR}
wget http://www.zlib.net/zlib-1.2.11.tar.gz
tar xzf zlib-1.2.11.tar.gz
cd zlib-1.2.11
CFLAGS=-fPIC ./configure
${MAKE}
cp -f *.h ${INCLUDEDIR}
cp -f libz.a ${LIBDIR}/libz.a


# libjpeg
cd ${TMPDIR}
wget http://www.ijg.org/files/jpegsrc.v9b.tar.gz
tar xzf jpegsrc.v9b.tar.gz
cd jpeg-9b
./configure --with-pic
${MAKE}
cp -f *.h ${INCLUDEDIR}
cp .libs/libjpeg.a ${LIBDIR}/libjpeg.a


# libtiff
cd ${TMPDIR}
wget http://download.osgeo.org/libtiff/tiff-4.0.8.tar.gz
tar xzf tiff-4.0.8.tar.gz
cd tiff-4.0.8
./configure --with-pic --disable-lzma --disable-jbig
${MAKE}
cp -f libtiff/*.h ${INCLUDEDIR}
cp -f libtiff/.libs/libtiff.a ${LIBDIR}/libtiff.a


# libIL (DevIL)
cd ${TMPDIR}
git clone --depth 1 https://github.com/DentonW/DevIL
cd DevIL/DevIL
#sudo apt-get install freeglut3-dev
cmake . -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_SHARED_LIBS=0 \
        -DPNG_PNG_INCLUDE_DIR=${INCLUDEDIR} -DPNG_LIBRARY_RELEASE=${LIBDIR}/libpng.a \
        -DJPEG_INCLUDE_DIR=${INCLUDEDIR} -DJPEG_LIBRARY=${LIBDIR}/libjpeg.a \
        -DTIFF_INCLUDE_DIR=${INCLUDEDIR} -DTIFF_LIBRARY_RELEASE=${LIBDIR}/libtiff.a \
        -DZLIB_INCLUDE_DIR=${INCLUDEDIR} -DZLIB_LIBRARY_RELEASE=${LIBDIR}/libz.a
${MAKE}
mkdir -p ${INCLUDEDIR}/IL
cp -f include/IL/*.h ${INCLUDEDIR}/IL
cp -f lib/x64/libIL.a ${LIBDIR}/libIL.a

# libunwind
cd ${TMPDIR}
wget http://download.savannah.nongnu.org/releases/libunwind/libunwind-1.2.1.tar.gz
tar xzf libunwind-1.2.1.tar.gz
cd libunwind-1.2.1
./configure --with-pic --disable-minidebuginfo
${MAKE}
cp -rf include ${INCLUDEDIR}
cp -f src/.libs/libunwind.a ${LIBDIR}/libunwind.a

# glew
wget https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0.tgz
tar xzf glew-2.1.0.tgz
cd glew-2.1.0
${MAKE}
cp -rf include ${INCLUDEDIR}
cp -f lib/libGLEW.a ${LIBDIR}/libGLEW.a
