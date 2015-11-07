#!/bin/bash
# -fno-rtti and -fno-exceptions turn off C++ features that 
# we don't use.

# Common variables to all builds
Internal_Debug="-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1"
CommonFlags="-Wall -Wextra -fno-rtti -fno-exceptions `sdl2-config --cflags --libs`"

pushd ../build
c++  $Internal_Debug ../code/sdl_handmade.cpp -o sdl_handmade -g $CommonFlags
popd


