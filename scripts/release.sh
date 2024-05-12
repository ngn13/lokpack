#!/bin/bash -e

# import common functions
if [ ! -f "scripts/common.sh" ]; then
  echo "You should run this script in the root of the repository"
  exit 1
fi

source scripts/common.sh

# vars
RELDIR="/tmp/lokpack-release"

# build the static binary
./scripts/build.sh -static

# copy the binaries and patch script
mkdir -p "$RELDIR"
mv dist/encryptor dist/decryptor "$RELDIR"
cp scripts/patch.py "$RELDIR"

# archive all
_echo "${BLUE}Creating the linux-x86_64 archive"
pushd "$RELDIR" > /dev/null
  tar czf "$RELDIR/lokpack-${VERSION}-x86_64.tar.gz" *
popd > /dev/null

mv "$RELDIR/lokpack-"*".tar.gz" .
rm -rf "$RELDIR"

_echo "${GREEN}Build completed"
