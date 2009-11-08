#! /bin/sh

set -e
cd $(dirname $0)/..

doxygen

chmod -R a+rx Documentation/doxygen
