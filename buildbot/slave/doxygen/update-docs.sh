#!/bin/sh

# Quit on error.
set -e

umask 022
doxygen

chmod go+x .. . doc doc/doxygen doc/doxygen/html
