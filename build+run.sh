#!/bin/bash
set -xe

CC=cc
CFLAGS="-Wall -Wextra"

# Build
$CC $CFLAGS -o png png.c

# Run
./png PNG_transparency_demonstration_1.png output.png