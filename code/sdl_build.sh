#!/bin/bash
pushd ../build
c++ ../code/sdl_handmade.cpp -o sdl_handmade -g `sdl2-config --cflags --libs`
popd


