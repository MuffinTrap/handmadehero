#!/bin/bash
pushd ../build
c++ -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 ../code/sdl_handmade.cpp -o sdl_handmade -g `sdl2-config --cflags --libs`
popd


