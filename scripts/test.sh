#!/bin/bash

# generates test directory with random files, stores the SHA256 hashes of the
# random files, encrypts the directory using the encryptor, then decrypts it
# using the decryptor and finally compares the stored hashes again to test if
# all the encryption and decryption was successful

# import common functions
if [ ! -f "scripts/common.sh" ]; then
  echo "You should run this script in the root of the repository"
  exit 1
fi

source scripts/common.sh

# check if encryptor and the decryptor is built or not
[ ! -f "dist/encryptor" ] && fail "Build the encryptor first" && exit 1
[ ! -f "dist/decryptor" ] && fail "Build the decryptor first" && exit 1

rand() {
  echo $(($1 + $RANDOM % ($2 + 1 - $1)))
}

verify() {
  check_hash "${1}" "${2}" && return 0
  fail "Hash verification failed for ${1}"
  exit 1
}

# cleanup and create the test directory
if [ -d test ] && ! rm -r test; then
  fail "Failed to remove the test directory"
  exit 1
fi

if ! mkdir -p test/files test/ftp; then
  fail "Failed to create the test directory"
  exit 1
fi

# generate random files and store their SHA1 hashes
pushd "./test/files" > /dev/null || exit 1

for i in $(seq 1 $(rand 5 20)); do
  name="$(openssl rand -hex 4).dat"
  case $i in
    1)
      # small file (42 bytes)
      head -c 42 /dev/urandom > "${name}"
      ;;

    2)
      # large file (1 GiB)
      head -c 1073741824 /dev/urandom > "${name}"
      ;;

    *)
      # random sized file (2 KiB - 1 MiB)
      head -c $(rand 2048 1048576) /dev/urandom > "${name}"
      ;;
  esac
  sums+=("$(sha256sum "${name}")")
done

popd > /dev/null || exit 1

# setup the FTP server
python3 -m pyftpdlib -w -d ./test/ftp &
ftppid=$!
sleep 6

# encrypt and decrypt files
pushd "./test/files" > /dev/null || exit 1
../../dist/encryptor --ftp-url=ftp://localhost:2121 --paths=. --exts=dat
../../dist/decryptor .
popd > /dev/null || exit 1

# stop the FTP server
kill -9 $ftppid

# verify the decrypted & uploaded files
for sum in "${sums[@]}"; do
  hash="$(echo "${sum}" | awk '{print $1}')"
  name="$(echo "${sum}" | awk '{print $2}')"

  verify "./test/files/${name}" "${hash}"
  verify "./test/ftp/${name}"   "${hash}"
done

success "Hash check was successful, all the files are intact"
exit 0
