#!/bin/bash

# takes an input file, replaces newlines with '\n' and prints the output

if [ $# -ne 1 ]; then
  echo "please specify a file path"
  exit 1
fi

if [ ! -f "${1}" ]; then
  echo "specified file is not accessible"
  exit 1
fi

while read -r line; do
  echo -n "${line}\n"
done < "${1}"
