#!/bin/bash -e

# import common functions
if [ ! -f "scripts/common.sh" ]; then
  echo "You should run this script in the root of the repository"
  exit 1
fi

source scripts/common.sh

# vars
VERSION="1.3"
PUB_TEMP="/tmp/lokpack_pub"
PRIV_TEMP="/tmp/lokpack_priv"
DEBUG_MODE="false"

# create key pair
_echo "${BLUE}Generating RSA key pair"
rm -f "$PUB_TEMP" "$PRIV_TEMP"
openssl genpkey -algorithm RSA -out "$PRIV_TEMP" -pkeyopt rsa_keygen_bits:4096 2> /dev/null
openssl rsa -in "$PRIV_TEMP" -pubout -out "$PUB_TEMP" 2> /dev/null

PRIV_KEY=$(sed -z 's/\n/\\n/g' < $PRIV_TEMP)
PUB_KEY=$(sed -z 's/\n/\\n/g' < $PUB_TEMP)

# flags
ENC_LIBS="-lpthread -lcrypto -lcurl"
DEC_LIBS="-lpthread -lcrypto"

if [[ "$1" == "-debug" ]]; then
  _echo "${BOLD}=========================================================="
  _echo "${BLUE}YOU ARE RUNNING THE BUILD IN ${BOLD}**DEBUG**${RESET}${BLUE} MODE"
  _echo "${BOLD}=========================================================="
  _echo "${RED}=> You won't be able to run the binaries on other machines"
  _echo "${RED}=> All debugging symbols will be kept"
  _echo "${RED}=> All optimizations are disabled"
  CFLAGS=""
  DEBUG_MODE="true"
elif [[ "$1" == "-static" ]]; then
  _echo "${BOLD}========================================================="
  _echo "${BLUE}YOU ARE RUNNING THE BUILD IN ${BOLD}**STATIC**${RESET}${BLUE} MODE"
  _echo "${BOLD}========================================================="
  _echo "${GREEN}=> You will be able to run the binaries on other machines"
  _echo "${GREEN}=> All debugging symbols will be stripped"
  _echo "${GREEN}=> All optimizations are enabled"
  CFLAGS="-O3 -s -static -L./static/usr/lib"
else
  _echo "${BOLD}=========================================================="
  _echo "${BLUE}YOU ARE RUNNING THE BUILD IN ${BOLD}**LOCAL**${RESET}${BLUE} MODE"
  _echo "${BOLD}=========================================================="
  _echo "${RED}=> You won't be able to run the binaries on other machines"
  _echo "${GREEN}=> All debugging symbols will be stripped"
  _echo "${GREEN}=> All optimizations are enabled"
  CFLAGS="-O3"
fi

# build
mkdir -pv dist

gcc $CFLAGS -o dist/encryptor     \
  -DVERSION=\"${VERSION}\"        \
  -DBUILD_PUB="\"${PUB_KEY}\""    \
  encryptor/*.c lib/*.c $ENC_LIBS

gcc $CFLAGS -o dist/decryptor     \
  -DVERSION=\"${VERSION}\"        \
  -DBUILD_PUB="\"${PUB_KEY}\""    \
  -DBUILD_PRIV="\"${PRIV_KEY}\""  \
  -DDEBUG_MODE=${DEBUG_MODE}      \
  decryptor/*.c lib/*.c $DEC_LIBS

# strip
if [[ "$1" != "debug" ]]; then
  strip --strip-unneeded dist/*
fi
