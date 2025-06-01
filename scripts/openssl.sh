#!/bin/bash -e

# import common functions
if [ ! -f "scripts/common.sh" ]; then
  echo "You should run this script in the root of the repository"
  exit 1
fi

source scripts/common.sh

# create build dir
mkdir -p dist/static && cd dist/static
static="${PWD}"

file="$(get_file "${openssl_url}")"
dir="${file%.tar.gz*}"

if [ -z "${file}" ] || [ -z "${dir}" ]; then
  fail "Failed to obtain file or directory name"
  exit 1
fi

# remove directory from previous build
rm -rf "${dir}"

# check if the archive is already downloaded
if [ ! -f "${file}" ] || ! check_hash "${file}" "${openssl_hash}"; then
  # remove the previous download
  rm -f "${file}"

  # download the archive
  info "Downloading openssl release archive"
  wget -q --show-progress "${openssl_url}"

  # verify the archive
  if ! check_hash "${file}" "${openssl_hash}"; then
    fail "Hash verification failed for ${file}"
    exit 1
  fi
fi

# extract the source archive
info "Extracting openssl release archive"
tar xf "${file}"

# configure and build
pushd "${dir}" > /dev/null
  info "Starting build (using all CPU cores)"
  ./Configure --prefix=/usr         \
              --openssldir=/etc/ssl \
              --libdir=lib          \
              no-shared             \
              no-docs
  make -j$(nproc)
  make DESTDIR="${static}" install
popd > /dev/null

success "Build successful, cleaning up"
rm "${file}" && rm -r "${dir}"
