#!/bin/sh

# Quit on error.
set -e

doxygen

chmod -R a+rx doc/doxygen
