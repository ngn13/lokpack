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
_echo "${BLUE}${BOLD}Downloading curl archive"
wget "$curl_url"
get_file "$curl_url"
check_hash "$file" "$curl_hash"

# extract the source archive
_echo "${BLUE}${BOLD}Extracting curl archive"
tar xf "$file"
dir="${file%.tar.xz*}"

# configure and build
pushd "$dir"
  _echo "${BLUE}${BOLD}Starting build (using all CPU cores)"
  ./configure --help
  ./configure --prefix=/usr            \
            --without-libssh2          \
            --without-nghttp2          \
            --without-brotli           \
            --without-libpsl           \
            --without-zstd             \
            --without-zlib             \
            --disable-ldap             \
            --without-zstd             \
            --without-libidn2          \
            --without-ssl              \
            --enable-threaded-resolver
  make -j$(nproc) && make DESTDIR="$static_path" install
popd

_echo "${GREEN}${BOLD}Build successful, cleaning up"
rm "$file" && rm -r "$dir"
