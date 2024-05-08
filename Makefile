LIB_C = $(wildcard lib/*.c)
LIB_H = $(wildcard lib/*.h)

ENC_C = $(wildcard encryptor/*.c) $(wildcard encryptor/*/*.c)
ENC_H = $(wildcard encryptor/*.h encryptor/*/*.h)

DEC_C = $(wildcard decryptor/*.c) $(wildcard decryptor/*/*.c)
DEC_H = $(wildcard decryptor/*.h decryptor/*/*.h)

CC = gcc

BUILD_PUB = "-----BEGIN PUBLIC KEY-----\nMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAyGwVR1UaI4AslL0XOugi\nnMG2I70N3X5cKHiOhUhbxMEH0GGAvnj0obCOyjy/92DwkwWc+bvTFHZf8wtFJZCe\nCcKA6RbtUyXy+1qlQU3JBhkTG8nJCUvDCbft1yd3ZFf9B7SU1ulS/x/+nhMUwBrB\no775PhdbJMLGrYw+dCmAjcLCeMm8TNQAvG7WDblaE/fbp1pULawld2OvkCEYPCTk\nPYznEFfBwiBatBBe9EcZYcqFinV6U5Gv8ToGVNGMQ1so9mTsciULvJ8sKwpMV3CY\nV/tYVJcbUj4cr/BoLIZVkC6JEGuVKCaxMPIIomK4kkUnAbNnkakXOy7erhC4UYEh\nEoEExp0GvnyePEcxWUe2dTXxY/Z4bviVvVf7sPzgMp4iGaZbF/9nuZIugrzf8Ssm\nFAuzP+iUJZKH40AoTT/jl18j7BZ9NPdsUDFfCPRQI0zy7a9ioOPzhWrf7PYIWHn0\njTUB3PUYylJb0OUGt8pSxJf/QKgqbz2bYn8jLLr/NwDEs5hAy+6hWZ5OiKwxlIEX\nj3M+DkyRgIDHjkpTd4TZ5SkCd+CEOp+PnL6mllhaxg/LL+gQjQhKrGCEAJ/3pPje\n2T9pmHSWnpHcx4xcCk8FY3V4c5G1AqHw6vAIF9q7yJF1cPbDjY2u9OtNgvIu+Qor\nP+p+35EwhoJXGuT4Cc0paLECAwEAAQ==\n-----END PUBLIC KEY-----\n"
BUILD_PRIV = "-----BEGIN PRIVATE KEY-----\n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\n40LwdZYStJV94gI4IS6/sxAFMRasNRA=\n-----END PRIVATE KEY-----\n"
VERSION = "1.2"
DEBUG = false

all: dist/encryptor dist/decryptor

dist/encryptor: $(ENC_C) $(ENC_H) $(LIB_C) $(LIB_H)
	mkdir -pv dist/
	$(CC) $(CFLAGS) $(ENC_C) $(LIB_C) -DVERSION=\"${VERSION}\" -DBUILD_PUB=\"${BUILD_PUB}\" -o $@ -lpthread -lcurl -lcrypto

dist/decryptor: $(DEC_C) $(DEC_H) $(LIB_C) $(LIB_H)
	mkdir -pv dist/
	$(CC) $(CFLAGS) $(DEC_C) $(LIB_C) -DDEBUG_MODE=${DEBUG} -DVERSION=\"${VERSION}\" -DBUILD_PUB=\"${BUILD_PUB}\" -DBUILD_PRIV=\"${BUILD_PRIV}\" -o $@ -lpthread -lcrypto

format:
	clang-format -i -style=file ./*/*.c ./*/*.h

.PHONY: format
