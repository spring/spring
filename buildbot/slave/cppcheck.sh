#!/bin/sh
set +e

# TODO: add suppressions instead of excluding rts/lib completely

find rts -name \*.cpp | grep -v '^rts/lib' | cppcheck --enable=all --quiet --file-list=/dev/stdin $@
