#!/bin/sh

# generates documentation from springs source code
set -e
. buildbot/slave/prepare.sh

DEST=${TMP_BASE}/doc/${REV}
mkdir -p $DEST/engine

echo $DEST
(
cat Doxyfile
echo "OUTPUT_DIRECTORY = $DEST/engine"
echo "HTML_OUTPUT = ./"
) | doxygen -
echo $DEST

#FIXME: find a way to directly generate in $DEST
cd AI/Interfaces/Java/bin
ant doc
cd -
mv dist/AI/Interfaces/Java/0.1/doc/jdoc/ $DEST/Java

cd AI/Wrappers/JavaOO/bin
ant doc
cd -
mv dist/AI/Wrappers/JavaOO/doc/jdoc/ $DEST/JavaOO

echo "Wrote doc to $DEST"

