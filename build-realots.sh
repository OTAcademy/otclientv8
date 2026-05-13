#!/usr/bin/env sh
set -eu

VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"

if [ ! -x "$VCPKG_ROOT/vcpkg" ]; then
	echo "Missing vcpkg at $VCPKG_ROOT." >&2
	echo "Install it with:" >&2
	echo "  git clone https://github.com/microsoft/vcpkg.git $VCPKG_ROOT" >&2
	echo "  $VCPKG_ROOT/bootstrap-vcpkg.sh" >&2
	exit 1
fi

"$VCPKG_ROOT/vcpkg" install
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j"$(nproc)"
cp build/otclient ./otclient
