#!/bin/bash

## launcher for 64-bit Spring 104.0.1-469-gecce754 and newer maintenance builds
## enables the Steam overlay by forced preloading, also resolves pr-downloader's
## libcurl3 dependency
SYSLIBPATH="lib/x86_64-linux-gnu"
USRLIBPATH="usr/${SYSLIBPATH}"

## system and user 64-bit libs
LIBPATHS="/${SYSLIBPATH}:/${USRLIBPATH}"

## Steam 64-bit libs; modify ubuntu* as necessary
STEAMHOME="$HOME/.local/share/Steam"
STEAMPATH="${STEAMHOME}/ubuntu12_32/steam-runtime/amd64"
STEAMLIBS="${STEAMPATH}/${SYSLIBPATH}:${STEAMPATH}/${USRLIBPATH}"

export LD_LIBRARY_PATH="${LIBPATHS}:${STEAMLIBS}:$LD_LIBRARY_PATH"
export LD_PRELOAD="${STEAMPATH}/${USRLIBPATH}/libcurl.so.3:${STEAMHOME}/ubuntu12_64/gameoverlayrenderer.so"

./spring $@

