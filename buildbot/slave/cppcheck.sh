#!/bin/sh
set +e

find rts AI/Wrappers AI/Interfaces -name *\.cpp -or -name *\.c \
|grep -v '^rts/lib' \
|cppcheck --force --enable=style,information,unusedFunction --quiet -j2 -I rts --file-list=/dev/stdin --suppressions-list=buildbot/slave/cppcheck.supress $@
