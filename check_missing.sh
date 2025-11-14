#!/usr/bin/env bash

if [ ! -e parser ]; then
	make parser
fi

./parser "$1" 2>&1 > /dev/null | sort | uniq | grep -E 'not implemented | Unrecognized'
