#!/bin/bash -e

# import common functions
if [ ! -f "scripts/common.sh" ]; then
  echo "You should run this script in the root of the repository"
  exit 1
fi

source scripts/common.sh

# vars
privfile="/tmp/lokpack_priv"
pubfile="/tmp/lokpack_pub"

if [ -z "${1}" ]; then
  fail "Please specify a build mode"
  exit 1
fi

# flags
case "${1}" in
  "-debug")
    print "${BOLD}=========================================================="
    print "${BLUE}YOU ARE RUNNING THE BUILD IN ${BOLD}**DEBUG**${RESET}${BLUE} MODE"
    print "${BOLD}=========================================================="
    print "${RED}=> You won't be able to run the binaries on other machines"
    print "${RED}=> All debugging symbols will be kept"
    print "${RED}=> All optimizations are disabled"
    flags=""
    debug=1
    ;;

  "-static")
    print "${BOLD}========================================================="
    print "${BLUE}YOU ARE RUNNING THE BUILD IN ${BOLD}**STATIC**${RESET}${BLUE} MODE"
    print "${BOLD}========================================================="
    print "${GREEN}=> You will be able to run the binaries on other machines"
    print "${GREEN}=> All debugging symbols will be stripped"
    print "${GREEN}=> All optimizations are enabled"
    flags="-O3 -s -static -L./static/usr/lib"
    debug=0
    ;;

  "-local")
    print "${BOLD}=========================================================="
    print "${BLUE}YOU ARE RUNNING THE BUILD IN ${BOLD}**LOCAL**${RESET}${BLUE} MODE"
    print "${BOLD}=========================================================="
    print "${RED}=> You won't be able to run the binaries on other machines"
    print "${GREEN}=> All debugging symbols will be stripped"
    print "${GREEN}=> All optimizations are enabled"
    flags="-O3"
    debug=0
    ;;

  *)
    fail "Invalid build option: ${1}"
    exit 1
    ;;
esac

# create key pair
info "Generating RSA key pair"
rm -f "${pubfile}" "${privfile}"
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:8192 -out "${privfile}"
openssl rsa -pubout -in "${privfile}" -out "${pubfile}"

privkey=$(sed -z 's/\n/\\n/g' < "${privfile}")
pubkey=$(sed -z 's/\n/\\n/g'  < "${pubfile}" )

rm "${privfile}"
rm "${pubfile}"

# build
make EXTRAFLAGS="${flags}" \
  DEBUG=${debug} PUBKEY="${pubkey}" PRIVKEY="${privkey}"

# strip
[ $debug -eq 1 ] && strip --strip-unneeded dist/*
