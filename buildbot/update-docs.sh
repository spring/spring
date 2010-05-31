#!/bin/sh

# Quit on error.
set -e

cd $(dirname $0)/..

doxygen

chmod -R a+rx doc/doxygen
