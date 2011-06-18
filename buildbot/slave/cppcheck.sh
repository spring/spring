#!/bin/sh
set +e

# TODO: add suppressions instead of excluding rts/lib completely

INCLUDE="-I rts -I rts/System"

find rts AI/Wrappers AI/Interfaces -name *\.cpp -or -name *\.c \
|grep -v '^rts/lib' \
|cppcheck --enable=style,information,unusedFunction --quiet -j2 ${INCLUDE} --file-list=/dev/stdin $@
