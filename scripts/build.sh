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
    print "${BOLD}${BLUE}=========================================================="
    print "${BOLD}YOU ARE RUNNING THE BUILD IN ${BLUE}DEBUG${RESET}${BOLD} MODE"
    print "${BOLD}${BLUE}=========================================================="

    print "${BOLD}${RED}=>${RESET} You won't be able to run the binaries on other machines"
    print "${BOLD}${RED}=>${RESET} All debugging symbols will be kept"
    print "${BOLD}${RED}=>${RESET} All optimizations are disabled"

    flags=""
    debug=1
    ;;

  "-static")
    print "${BOLD}${BLUE}=========================================================="
    print "${BOLD}YOU ARE RUNNING THE BUILD IN ${BLUE}STATIC${RESET}${BOLD} MODE"
    print "${BOLD}${BLUE}=========================================================="

    print "${BOLD}${GREEN}=>${RESET} You will be able to run the binaries on other machines"
    print "${BOLD}${GREEN}=>${RESET} All debugging symbols will be stripped"
    print "${BOLD}${GREEN}=>${RESET} All optimizations are enabled"

    flags="-O3 -s -static -L./dist/static/usr/lib"
    debug=0
    ;;

  "-local")
    print "${BOLD}${BLUE}=========================================================="
    print "${BOLD}YOU ARE RUNNING THE BUILD IN ${BLUE}LOCAL${RESET}${BOLD} MODE"
    print "${BOLD}${BLUE}=========================================================="

    print "${BOLD}${RED}=>${RESET} You won't be able to run the binaries on other machines"
    print "${BOLD}${GREEN}=>${RESET} All debugging symbols will be stripped"
    print "${BOLD}${GREEN}=>${RESET} All optimizations are enabled"

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
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:8192 \
  -out "${privfile}" &> /dev/null
openssl rsa -pubout -in "${privfile}" -out "${pubfile}" &> /dev/null

privkey="$(./scripts/nonl.sh "${privfile}")"
pubkey="$(./scripts/nonl.sh "${pubfile}")"

rm "${privfile}"
rm "${pubfile}"

# build
info "Building the binaries"
make clean > /dev/null
make EXTRAFLAGS="${flags}" \
  LP_DEBUG=${debug} LP_PUBKEY="${pubkey}" LP_PRIVKEY="${privkey}"

# strip
if [ $debug -ne 1 ]; then
  info "Stripping the binaries"
  strip --strip-unneeded dist/encryptor
  strip --strip-unneeded dist/decryptor
fi

success "Binaries are ready, see the dist directory"
