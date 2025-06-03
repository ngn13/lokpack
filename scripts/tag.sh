#!/bin/bash -e

# import common functions
if [ ! -f "scripts/common.sh" ]; then
  echo "You should run this script in the root of the repository"
  exit 1
fi

source scripts/common.sh

git tag "v${VERSION}" && success "Tagged the version ${VERSION}" && exit 0

fail "Failed to tag the version ${VERSION}"
exit 1
