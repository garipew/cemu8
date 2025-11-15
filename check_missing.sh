#!/usr/bin/env bash

if [ ! -e parser ]; then
	make parser
fi

missing=$(./parser "$1" 2>&1 >/dev/null) 

echo $missing | sort | uniq | grep -E 'not implemented|Unrecognized'
