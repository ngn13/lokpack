#!/bin/bash -e

# import common functions
if [ ! -f "scripts/common.sh" ]; then
  echo "You should run this script in the root of the repository"
  exit 1
fi

source scripts/common.sh

# create build dir
mkdir -p static && cd static
static_path="$PWD"

# download & verify the source archive
_echo "${BLUE}${BOLD}Downloading openssl archive"
wget "$openssl_url"
get_file "$openssl_url"
check_hash "$file" "$openssl_hash"

# extract the source archive
_echo "${BLUE}${BOLD}Extracting openssl archive"
tar xf "$file"

# configure and build
dir="${file%.tar.gz*}"
pushd "$dir"
  _echo "${BLUE}${BOLD}Starting build (using all CPU cores)"
  ./config --prefix=/usr       \
         --openssldir=/etc/ssl \
         --libdir=lib          \
         zlib-dynamic
  make -j$(nproc)
  make DESTDIR="$static_path" install
popd

_echo "${GREEN}${BOLD}Build successful, cleaning up"
rm "$file" && rm -r "$dir"
