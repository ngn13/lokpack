#!/usr/bin/python3

from Crypto.PublicKey import RSA
from hashlib import sha256
from os import path
import re


def success(text: str) -> None:
    print(f"\x1b[32;1m[+]\x1b[0m {text}")


def info(text: str) -> None:
    print(f"\x1b[34;1m[*]\x1b[0m {text}")


def fail(text: str) -> None:
    print(f"\x1b[31;1m[!]\x1b[0m {text}")


class Patcher:
    def __init__(self):
        self.binaries = ["encryptor", "decryptor"]
        self.pub_regex = (
            b"-----BEGIN PUBLIC KEY-----"
            + b"([0-9A-Za-z]|\\n|\\r|\\/|=|\\+)*"
            + b"-----END PUBLIC KEY-----\\n"
        )
        self.priv_regex = (
            b"-----BEGIN RSA PRIVATE KEY-----"
            + b"([0-9A-Za-z]|\\n|\\r|\\/|=|\\+)*"
            + b"-----END RSA PRIVATE KEY-----\\n"
        )
        self.gen_rsa()

    def gen_rsa(self) -> None:
        info("Generating an 8192 bit RSA key (may take a second)")
        rsa_key = RSA.generate(8192)
        self.pub_key = rsa_key.public_key().export_key() + b"\n"
        self.priv_key = rsa_key.export_key() + b"\n"
        success(f"Public key SHA256: {sha256(self.pub_key).hexdigest()}")

    def patch_all(self) -> None:
        found = False

        for b in self.binaries:
            if not path.exists(b) or not path.isfile(b):
                continue

            found = True

            if not self.patch_binary(b):
                fail(f"Failed to patch binary: {b}")
                continue

            success(f"Patched binary: {b}")

        if not found:
            fail("There is no binary to patch")

    def patch_binary(self, fp: str) -> bool:
        try:
            b = open(fp, "rb")
            binary = b.read()
            b.close()
        except Exception:
            return False

        length = len(binary)

        binary = re.sub(self.pub_regex, self.pub_key, binary)
        binary = re.sub(self.priv_regex, self.priv_key, binary)

        if length != len(binary):
            return False

        try:
            b = open(fp, "wb")
            b.write(binary)
            b.close()
        except Exception:
            return False

        return True


if __name__ == "__main__":
    patcher = Patcher()
    patcher.patch_all()
