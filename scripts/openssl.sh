#!/bin/bash -e

source scripts/common.sh
mkdir -p static && cd static
static_path="$PWD"

echo "[*] Downloading openssl archive"
wget "$openssl_url"
get_file "$openssl_url"
check_hash "$file" "$openssl_hash"
echo "[*] Extracting openssl archive"
tar xf "$file"

dir="${file%.tar.gz*}"
pushd "$dir"
  echo "[*] Starting build (using all CPU cores)"
  ./config --prefix=/usr       \
         --openssldir=/etc/ssl \
         --libdir=lib          \
         zlib-dynamic
  make -j$(nproc)
  make DESTDIR="$static_path" install
popd

echo "[+] Build successful, cleaning up"
rm "$file" && rm -r "$dir"
