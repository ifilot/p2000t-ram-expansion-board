#!/bin/bash

#
# Compiles the RAMTESTER binary using the z88dk docker
#
# Requires a working installation of Docker
#
if [[ "$OSTYPE" == "msys" ]]; then
    winpty docker run -v `pwd | sed 's/\//\/\//g'`://src/ -it z88dk/z88dk make
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    docker run -v `pwd | sed 's/\//\/\//g'`://src/ -it z88dk/z88dk make
else
    echo "Unknown operating system"
fi