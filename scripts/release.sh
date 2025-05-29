#!/bin/bash -e

# import common functions
if [ ! -f "scripts/common.sh" ]; then
  echo "You should run this script in the root of the repository"
  exit 1
fi

source scripts/common.sh

# vars
reldir="/tmp/lokpack-release"

# build the static binary
./scripts/build.sh -static

# create the release directory
info "creating the release directory"
mkdir -p "${reldir}"

# copy the binaries and patch script
info "copying the release files"
mv dist/encryptor dist/decryptor "${reldir}"
cp scripts/patch.py "${reldir}"

# archive all
info "creating the release archive"
pushd "${reldir}" > /dev/null
  tar czf "${reldir}/lokpack-${VERSION}.tar.gz" *
popd > /dev/null

mv "${reldir}/lokpack-"*".tar.gz" .
rm -r "${reldir}"

success "created the release archive"
