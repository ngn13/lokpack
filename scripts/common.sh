#!/bin/bash

openssl_url="https://www.openssl.org/source/openssl-3.1.2.tar.gz"
openssl_hash="1d7861f969505e67b8677e205afd9ff4"

curl_url="https://curl.se/download/curl-8.2.1.tar.xz"
curl_hash="556576a795bdd2c7d10de6886480065f"

get_file() {
  file=$(echo $1 | rev | cut -d "/" -f 1 | rev)
}

check_hash() {
  if ! echo "$2 $1" | md5sum -c > /dev/null; then
    echo "[-] Hash verification for $1 failed!"
    exit 1
  fi
  echo "[+] Hash verification success!"
}
