#!/bin/bash -e

source scripts/common.sh
mkdir -p static && cd static
static_path="$PWD"

echo "[*] Downloading curl archive"
wget "$curl_url"
get_file "$curl_url"
check_hash "$file" "$curl_hash"
echo "[*] Extracting curl archive"
tar xf "$file"

dir="${file%.tar.xz*}"
pushd "$dir"
  echo "[*] Starting build (using all CPU cores)"
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

echo "[+] Build successful, cleaning up"
rm "$file" && rm -r "$dir"
