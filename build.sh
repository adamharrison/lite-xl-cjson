#!/bin/bash

: ${CC=gcc}
: ${BIN=cjson.so}

CFLAGS="$CFLAGS -Ilib/lite-xl/resources/include -fPIC -shared"

[[ "$@" == "clean" ]] && rm -rf $BIN && exit 0
$CC $CFLAGS *.c -o $BIN $@
