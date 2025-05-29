#!/bin/bash -e

# import common functions
if [ ! -f "scripts/common.sh" ]; then
  echo "You should run this script in the root of the repository"
  exit 1
fi

source scripts/common.sh

# create build dir
mkdir -p dist/static && cd dist/static
static="${PWD}"

# download & verify the source archive
info "downloading openssl release archive"
wget "${openssl_url}"
get_file "${openssl_url}"
check_hash "${file}" "${openssl_hash}"

# extract the source archive
info "extracting openssl release archive"
tar xf "${file}"
dir="${file%.tar.gz*}"

# configure and build
pushd "${dir}"
  info "starting build (using all CPU cores)"
  ./config --prefix=/usr         \
           --openssldir=/etc/ssl \
           --libdir=lib          \
           zlib-dynamic
  make -j$(nproc)
  make DESTDIR="${static}" install
popd

success "build successful, cleaning up"
rm "${file}" && rm -r "${dir}"
