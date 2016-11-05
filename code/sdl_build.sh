#!/bin/bash
# -fno-rtti and -fno-exceptions turn off C++ features that 
# we don't use.

# Common variables to all builds
Internal_Debug="-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1"
#Internal_Debug=""
CommonFlags="-fno-rtti -fno-exceptions -Wall -Wextra `sdl2-config --cflags --libs`"

pushd ../build
#rm sdl_handmade
c++  $Internal_Debug ../code/sdl_handmade.cpp -o sdl_handmade -g $CommonFlags
popd


