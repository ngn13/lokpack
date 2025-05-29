#!/bin/bash

VERSION="1.3"

RED="\e[31m"
GREEN="\e[32m"
BLUE="\e[34m"
BOLD="\e[1m"
RESET="\e[0m"

print() {
  echo -e "${1}${RESET}"
}

fail(){
  print "${BOLD}${RED}[!]${RESET} ${BOLD}${1}"
}

success() {
  print "${BOLD}${GREEN}[+]${RESET} ${BOLD}${1}"
}

info(){
  print "${BOLD}${BLUE}[*]${RESET} ${BOLD}${1}"
}

openssl_url="https://www.openssl.org/source/openssl-3.5.0.tar.gz"
openssl_hash="344d0a79f1a9b08029b0744e2cc401a43f9c90acd1044d09a530b4885a8e9fc0"

curl_url="https://curl.se/download/curl-8.13.0.tar.gz"
curl_hash="c261a4db579b289a7501565497658bbd52d3138fdbaccf1490fa918129ab45bc"

get_file() {
  file=$(echo "${1}" | rev | cut -d/ -f1 | rev)
}

check_hash() {
  hash="$(sha256sum "${1}" | awk '{print $1}')"

  if [[ "${hash}" == "${2}" ]]; then
    success "Hash verification was sucessful for ${1}"
    return 0
  fi

  fail "Hash verification for ${1} failed"
  return 1
}
