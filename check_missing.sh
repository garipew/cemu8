#!/usr/bin/env bash

if [ ! -e chip8 ]; then
	make
fi

./chip8 "$1" 2>&1 > /dev/null | sort | uniq | grep -E 'not implemented | Unrecognized'
