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

file="$(get_file "${curl_url}")"
dir="${file%.tar.gz*}"

if [ -z "${file}" ] || [ -z "${dir}" ]; then
  fail "Failed to obtain file or directory name"
  exit 1
fi

# remove directory from previous build
rm -rf "${dir}"

# check if the archive is already downloaded
if [ ! -f "${file}" ] || ! check_hash "${file}" "${curl_hash}"; then
  # remove the previous download
  rm -f "${file}"

  # download the archive
  info "Downloading curl release archive"
  wget -q --show-progress "${curl_url}"

  # verify the archive
  if ! check_hash "${file}" "${curl_hash}"; then
    fail "Hash verification failed for ${file}"
    exit 1
  fi
fi

# extract the source archive
info "Extracting curl release archive"
tar xf "${file}"

# configure and build
pushd "${dir}" > /dev/null
  info "Starting build (using all CPU cores)"
  export CPPFLAGS="-I${static}/usr/include"
  export LDFLAGS="-L${static}/usr/lib"
  ./configure --prefix=/usr     \
              --disable-shared  \
              --disable-manual  \
              --without-libssh2 \
              --without-nghttp2 \
              --without-brotli  \
              --without-libpsl  \
              --without-zstd    \
              --without-zlib    \
              --disable-ldap    \
              --without-zstd    \
              --without-libidn2 \
              --with-openssl    \
              --enable-threaded-resolver
  make -j$(nproc)
  make DESTDIR="${static}" install
popd > /dev/null

success "Build successful, cleaning up"
rm "${file}" && rm -r "${dir}"
