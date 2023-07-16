CFLAGS="-O3 -Wall `pkg-config --cflags raylib`"
LIBS="-lm `pkg-config --libs raylib`"

clang $CFLAGS -o autocolor autocolor.c $LIBS
