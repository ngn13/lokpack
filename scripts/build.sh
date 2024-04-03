#!/bin/bash

# vars
VERSION="1.0"
KEY=$(openssl rand -hex 222 | md5sum | awk '{print $1}')
IV=$(openssl rand -hex 222 | md5sum | awk '{print $1}')

# flags
ENC_LIBS="-lcrypto -lcurl -lpthread"
DEC_LIBS="-lcrypto"
CFLAGS="-static -L./static/usr/lib"

# build & strip
mkdir -pv dist

gcc $CFLAGS -o dist/encryptor \
  -DVERSION=\"${VERSION}\"    \
  -DBUILD_KEY=\"${KEY}\"      \
  -DBUILD_IV=\"${BUILD_IV}\"  \
  encryptor/*.c lib/*.c $ENC_LIBS

gcc $CFLAGS -o dist/decryptor \
  -DVERSION=\"${VERSION}\"    \
  -DBUILD_KEY=\"${KEY}\"      \
  -DBUILD_IV=\"${BUILD_IV}\"  \
  decryptor/*.c lib/*.c $DEC_LIBS

strip --strip-unneeded dist/*
