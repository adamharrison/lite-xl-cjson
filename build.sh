#!/bin/bash

: ${CC=gcc}
: ${BIN=cjson.so}

CFLAGS="$CFLAGS -Ilib/lite-xl/resources/include -Ilib/cJSON -fPIC -shared"

[[ "$@" == "clean" ]] && rm -rf $BIN && exit 0
$CC $CFLAGS *.c -o $BIN $@
