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
      openssl rand 42 > "${name}"
      ;;

    2)
      openssl rand 1073741824 > "${name}"
      ;;

    *)
      openssl rand $(rand 2048 55555) > "${name}"
      ;;
  esac
  sums+=("$(sha256sum "${name}")")
done

popd > /dev/null || exit 1

# setup the FTP server
python3 -m pyftpdlib -w -d ./test/ftp &
ftppid=$!
sleep 3

# encrypt and decrypt files
pushd "./test/files" > /dev/null || exit 1
../../dist/encryptor --ftp-url=ftp://localhost:2121 --paths=. --exts=dat
../../dist/decryptor .
popd > /dev/null || exit 1

# stop the FTP server
kill -9 $ftppid

# check the decrypted & uploaded files
for sum in "${sums[@]}"; do
  hash="$(echo "${sum}" | awk '{print $1}')"
  name="$(echo "${sum}" | awk '{print $2}')"

  check_hash "./test/files/${name}" "${hash}" || exit 1
  check_hash "./test/ftp/${name}" "${hash}" || exit 1
done

success "Hash check was successful, all the files are intact"
exit 0
