#!/bin/bash

# Setting system dependent vars
BOOST_DIR=/tmp/boost/
BOOST_BUILD_DIR=/tmp/build-boost/
MINGWLIBS_DIR=/tmp/mingwlibs/

# spring's boost dependencies
BOOST_LIBS="test thread system filesystem regex program_options signals"
SPRING_HEADERS="${BOOST_LIBS} format ptr_container spirit algorithm date_time asio"
ASSIMP_HEADERS="math/common_factor smart_ptr"
BOOST_HEADERS="${SPRING_HEADERS} ${ASSIMP_HEADERS}"
BOOST_CONF=./user-config.jam

# x86 or x86_64
MINGW_GPP=/usr/bin/i686-mingw32-g++
MINGW_RANLIB=i686-mingw32-ranlib


BOOST_LIBS_ARG=""
for LIB in $BOOST_LIBS
do
	BOOST_LIBS_ARG="${BOOST_LIBS_ARG} --with-${LIB}"
done


#############################################################################################################

echo "\n---------------------------------------------------"
echo "-- clone git repo"
#git clone -n . ${MINGWLIBS_DIR}


# Setup final structure
echo "\n---------------------------------------------------"
echo "-- setup dirs"
mkdir -p ${BOOST_DIR} 2>/dev/null
rm -f ${MINGWLIBS_DIR}lib/libboost* 2>/dev/null
rm -Rf ${MINGWLIBS_DIR}include/boost 2>/dev/null
mkdir -p ${MINGWLIBS_DIR}lib/ 2>/dev/null
mkdir -p ${MINGWLIBS_DIR}include/boost/ 2>/dev/null


# Gentoo related - retrieve boost's tarball
echo "\n---------------------------------------------------"
echo "-- fetching boost's tarball"
command -v emerge >/dev/null 2>&1 || { echo >&2 "Gentoo needed. Aborting."; exit 1; } 
emerge boost --fetchonly &>/dev/null
source /etc/make.conf
find ${DISTDIR} -iname "boost_*.tar.*" -print 2>/dev/null | xargs tar -xa -C ${BOOST_DIR} -f


# bootstrap bjam
echo "\n---------------------------------------------------"
echo "-- bootstrap bjam"
cd ${BOOST_DIR}/boost_*
./bootstrap.sh || exit 1


# Building bcp - boosts own filtering tool
echo "\n---------------------------------------------------"
echo "-- creating bcp"
cd tools/bcp
../../bjam --build-dir=${BOOST_BUILD_DIR} || exit 1
cd ../..
cp $(ls ${BOOST_BUILD_DIR}/boost/*/tools/bcp/*/*/*/bcp) .


# Building the required libraries
echo "\n---------------------------------------------------"
echo "-- running bjam"
echo "-- using gcc : : ${MINGW_GPP} ;" > ${BOOST_CONF}
./bjam \
    --build-dir="${BOOST_BUILD_DIR}" \
    --stagedir="${MINGWLIBS_DIR}" \
    --user-config=${BOOST_CONF} \
    --debug-building \
    ${BOOST_LIBS_ARG} \
    variant=release \
    target-os=windows \
    threadapi=win32 \
    threading=multi \
    link=static \
    toolset=gcc \
|| exit 1

# fix library names (libboost_thread_win32.a -> libboost_thread-mt.a)
for f in $(ls ${MINGWLIBS_DIR}lib/*.a); do
	FIXEDBASENAME=$(basename "$f" | sed -e 's/_win32//' | sed -e 's/\.a/-mt\.a/' )
	mv "$f" "${MINGWLIBS_DIR}lib/$FIXEDBASENAME"
done


# Copying the headers to MinGW-libs
echo "\n---------------------------------------------------"
echo "-- copying headers"
rm -Rf ${BOOST_BUILD_DIR}/filtered
mkdir ${BOOST_BUILD_DIR}/filtered
./bcp ${BOOST_HEADERS} ${BOOST_BUILD_DIR}/filtered
cp -r ${BOOST_BUILD_DIR}/filtered/boost ${MINGWLIBS_DIR}include/boost


# some config we need
echo "\n---------------------------------------------------"
echo "-- adjust config/user.hpp"
echo "#define BOOST_THREAD_USE_LIB" >> "${MINGWLIBS_DIR}include/boost/config/user.hpp"


echo "\n---------------------------------------------------"
echo "-- adjust config/user.hpp"
cd ${MINGWLIBS_DIR}
#git add --all
#git commit -m "boost update"
#git push

# cleanup
#rm -rf ${BOOST_BUILD_DIR}
#rm -rf ${BOOST_DIR}
#rm -rf ${MINGWLIBS_DIR}
