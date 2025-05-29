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
info "Downloading curl release archive"
wget "${curl_url}"
get_file "${curl_url}"
check_hash "${file}" "${curl_hash}"

# extract the source archive
info "Extracting curl release archive"
tar xf "${file}"
dir="${file%.tar.xz*}"

# configure and build
pushd "${dir}"
  info "Starting build (using all CPU cores)"
  ./configure --prefix=/usr     \
              --without-libssh2 \
              --without-nghttp2 \
              --without-brotli  \
              --without-libpsl  \
              --without-zstd    \
              --without-zlib    \
              --disable-ldap    \
              --without-zstd    \
              --without-libidn2 \
              --without-ssl     \
              --enable-threaded-resolver
  make -j$(nproc)
  make DESTDIR="${static}" install
popd

success "Build successful, cleaning up"
rm "${file}" && rm -r "${dir}"
