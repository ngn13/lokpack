LIB_C = $(wildcard lib/*.c)
LIB_H = $(wildcard lib/*.h)

ENC_C = $(wildcard encryptor/*.c) $(wildcard encryptor/*/*.c)
ENC_H = $(wildcard encryptor/*.h encryptor/*/*.h)

DEC_C = $(wildcard decryptor/*.c) $(wildcard decryptor/*/*.c)
DEC_H = $(wildcard decryptor/*.h decryptor/*/*.h)

CC = gcc

BUILD_PUB = "$(shell cat keys/pub)"
BUILD_PRIV = "$(shell cat keys/priv)"

VERSION = $(shell cat scripts/common.sh | grep VERSION | cut -d "=" -f2)
DEBUG = false

all: dist/encryptor dist/decryptor

dist/encryptor: $(ENC_C) $(ENC_H) $(LIB_C) $(LIB_H)
	@mkdir -pv dist/
	@$(CC) $(CFLAGS) $(ENC_C) $(LIB_C) -DDEBUG=${DEBUG} -DVERSION=\"${VERSION}\" -DBUILD_PUB=\"${BUILD_PUB}\" -o $@ -lpthread -lcurl -lcrypto

dist/decryptor: $(DEC_C) $(DEC_H) $(LIB_C) $(LIB_H)
	@mkdir -pv dist/
	@$(CC) $(CFLAGS) $(DEC_C) $(LIB_C) -DDEBUG=${DEBUG} -DVERSION=\"${VERSION}\" -DBUILD_PUB=\"${BUILD_PUB}\" -DBUILD_PRIV=\"${BUILD_PRIV}\" -o $@ -lpthread -lcrypto

format:
	clang-format -i -style=file ./*/*.c ./*/*.h

.PHONY: format
