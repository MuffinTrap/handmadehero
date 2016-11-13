#!/bin/bash
# -fno-rtti and -fno-exceptions turn off C++ features that 
# we don't use.

# Common variables to all builds
Internal_Debug="-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1"
#Internal_Debug=""
CommonFlags="-fno-rtti -fno-exceptions -Wall -Wextra `sdl2-config --cflags --libs`"
NoWarnings="-Wno-unused-function -Wno-unused-parameter"


# We compile the cross-platform game code to dynamically linked library
# which is called a shared library on linux.
# The compiler flag -fpic, which mean position independent code does this.
# The compiler flag -shared compiles object file(s) to a library .so
# Linking to library is done with -l<libraryName>
# And we need to tell where the library is with -L<path>
# since the library is right here, just use -L.

# Also the loader needs to find library, use rpath
# by -Wl, -rpath=<path>

# But when program is run from Kdevelop the working directory is different,
# it is ../data, so we build the library there
# Instructions from http://www.cprogramming.com/tutorial/shared-libraries-linux-gcc.html

# How to load the library in code
# http://www.dwheeler.com/program-library/Program-Library-HOWTO/x172.html
# http://www.tldp.org/HOWTO/C++-dlopen/theproblem.html

LinkDynamicLinker="-ldl"
LinkHandmade="-L../data -Wl,-rpath="../data" -lhandmade"

pushd ../build
#rm sdl_handmade

#Library
c++  $Internal_Debug -c -fpic ../code/handmade.cpp -g $CommonFlags $NoWarnings
c++ -shared -o ../data/libhandmade.so handmade.o

c++  $Internal_Debug -c ../code/sdl_handmade.cpp -g $CommonFlags $NoWarnings

c++  $Internal_Debug sdl_handmade.o -o sdl_handmade $LinkDynamicLinker $LinkHandmade -g $CommonFlags $NoWarnings
popd


