#!/bin/bash
export MAGICK_HOME="$( pwd )/ext_app/ImageMagick-6.4.2"
export DYLD_LIBRARY_PATH="$MAGICK_HOME/lib"
export WGET_HOME="$( pwd )/ext_app/wget"
export PATH="$MAGICK_HOME/bin:$WGET_HOME:$PATH"
