#!/bin/env bash

set -e

FILES="main.c lim.c"
LIBS="-lpcap"

case "$1" in
    "test")
        gcc $FILES $LIBS
        ./a.out
        ;;
    "build")
        gcc $FILES $LIBS -DNDEBUG
        ./a.out "$2" "$2.out" "$3"
        ;;
    *)
        echo "$0: { test | build <in_file> <bps_rate> }"
        ;;
esac
