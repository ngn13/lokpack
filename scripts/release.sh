#!/bin/bash -e

# import common functions
if [ ! -f "scripts/common.sh" ]; then
  echo "You should run this script in the root of the repository"
  exit 1
fi

source scripts/common.sh

if [[ "$1" != "linux" ]] && [[ "$1" != "windows" ]]; then
  _echo "${RED}Please specify a valid release target (linux or windows)"
  exit 1
fi

# vars
TEMP=/tmp/lokpack-release
MINGW_BIN="/usr/x86_64-w64-mingw32/bin"
DLLS=(
  "libcrypto-3-x64.dll"
  "libcurl-4.dll"
  "libiconv-2.dll"
  "libnghttp2-14.dll"
  "libpsl-5.dll"
  "libssh2.dll"
  "libssl-3-x64.dll"
  "libstdc++-6.dll"
  "libunistring-5.dll"
  "libwinpthread-1.dll"
  "libzstd.dll"
  "libbrotlidec.dll"
  "libbrotlienc.dll"
  "libbrotlicommon.dll"
  "libidn2-0.dll"
  "libatomic-1.dll"
  "libcharset-1.dll"
  "libgcc_s_seh-1.dll"
  "libgfortran-5.dll"
  "libgomp-1.dll"
  "libobjc-4.dll"
  "libquadmath-0.dll"
  "libssp-0.dll"
  "zlib1.dll"
)

# build for the specific target
if [[ "$1" == "linux" ]]; then
  ./scripts/build.sh -static

  mkdir -p "$TEMP/linux"
  mv dist/encryptor dist/decryptor "$TEMP/linux"
  cp scripts/patch.py "$TEMP/linux"

  _echo "${BLUE}Creating the linux-x86_64 archive"
  pushd "$TEMP/linux" > /dev/null
    tar czf "$TEMP/lokpack-${VERSION}_linux-x86_64.tar.gz" *
  popd > /dev/null
else
  CC=x86_64-w64-mingw32-gcc ./scripts/build.sh

  mkdir -p "$TEMP/windows/dlls"
  mv dist/*.exe "$TEMP/windows"
  cp scripts/patch.py "$TEMP/windows"

  for d in "${DLLS[@]}"; do
    cp "$MINGW_BIN/$d" "$TEMP/windows/dlls"
  done

  _echo "${BLUE}Creating the windows-x86_64 archive"
  pushd "$TEMP/windows" > /dev/null
    tar czf "$TEMP/lokpack-${VERSION}_windows-x86_64.tar.gz" *
  popd > /dev/null
fi

mv "$TEMP/lokpack-"*".tar.gz" .
_echo "${GREEN}Completed build for $1"
