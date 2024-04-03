LIB_C = $(wildcard lib/*.c)
LIB_H = $(wildcard lib/*.h)

ENC_C = $(wildcard encryptor/*.c) $(wildcard encryptor/*/*.c)
ENC_H = $(wildcard encryptor/*.h encryptor/*/*.h)

DEC_C = $(wildcard decryptor/*.c) $(wildcard decryptor/*/*.c)
DEC_H = $(wildcard decryptor/*.h decryptor/*/*.h)

CC = gcc

BUILD_KEY = 60b725f10c9c85c70d97880dfe8191b3
BUILD_IV  = 60b725f10c9c85c70d97880dfe8191b3

all: dist/encryptor dist/decryptor

dist/encryptor: $(ENC_C) $(ENC_H) $(LIB_C) $(LIB_H)
	mkdir -pv dist/
	$(CC) $(CFLAGS) $(ENC_C) $(LIB_C) -DVERSION=\"${VERSION}\" -DBUILD_KEY=\"${BUILD_KEY}\" -DBUILD_IV=\"${BUILD_IV}\" -o $@ -lcurl -lpthread -lcrypto 

dist/decryptor: $(DEC_C) $(DEC_H) $(LIB_C) $(LIB_H)
	mkdir -pv dist/
	$(CC) $(CFLAGS) $(DEC_C) $(LIB_C) -DVERSION=\"${VERSION}\" -DBUILD_KEY=\"${BUILD_KEY}\" -DBUILD_IV=\"${BUILD_IV}\" -o $@ -lcrypto 

format:
	clang-format -i -style=file ./*/*.c ./*/*.h

.PHONY: format
