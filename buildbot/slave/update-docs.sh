#!/bin/sh

# Quit on error.
set -e

umask 0022
doxygen
