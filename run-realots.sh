#!/usr/bin/env sh
set -eu

if [ ! -x ./otclient ]; then
	echo "Missing ./otclient. Run ./build-realots.sh first." >&2
	exit 1
fi

exec ./otclient
