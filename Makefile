# commands & flags
CC = gcc

CFLAGS  = -Wall -Wextra -Werror -std=gnu89 -pedantic
CFLAGS += -Wno-overlength-strings # public/private key is larger than 509 bytes
CFLAGS += -fstack-protector-strong -fcf-protection=full -fstack-clash-protection
CFLAGS += -z noexecstack

INC  = -I./inc
LIBS = -lpthread -lcurl -lcrypto

# sources
LIB_C = $(wildcard src/lib/*.c)
LIB_H = $(wildcard inc/lib/*.h)

ENC_C = $(wildcard src/encryptor/*.c)
ENC_H = $(wildcard inc/encryptor/*.h)

DEC_C = $(wildcard src/decryptor/*.c)
DEC_H = $(wildcard inc/decryptor/*.h)

# compile time options
LP_DEBUG   = 0
LP_VERSION = $(shell cat scripts/common.sh | grep VERSION | cut -d "=" -f2)
LP_THREADS = 20
LP_PUBKEY  = $(shell sed -z 's/\n/\\n/g' < keys/public )
LP_PRIVKEY = $(shell sed -z 's/\n/\\n/g' < keys/private)

all: dist/encryptor dist/decryptor

dist/encryptor: $(ENC_C) $(ENC_H) $(LIB_C) $(LIB_H)
	@mkdir -pv dist/
	@$(CC) $(EXTRAFLAGS) $(CFLAGS) $(INC)                   \
		-DLP_DEBUG=${LP_DEBUG} -DLP_VERSION=\"${LP_VERSION}\" \
		-DLP_PUBKEY=\""${LP_PUBKEY}"\" -ULP_PRIVKEY           \
		$(ENC_C) $(LIB_C) -o $@ $(LIBS)

dist/decryptor: $(DEC_C) $(DEC_H) $(LIB_C) $(LIB_H)
	@mkdir -pv dist/
	@$(CC) $(EXTRAFLAGS) $(CFLAGS) $(INC)                             \
		-DLP_DEBUG=${LP_DEBUG} -DLP_VERSION=\"${LP_VERSION}\"           \
		-DLP_PUBKEY=\""${LP_PUBKEY}"\" -DLP_PRIVKEY=\""${LP_PRIVKEY}"\" \
		-DLP_THREADS=${LP_THREADS}                                      \
		$(DEC_C) $(LIB_C) -o $@ $(LIBS)

format:
	clang-format -i -Werror -style=file \
		$(LIB_C) $(LIB_H) $(ENC_C) $(ENC_H) $(DEC_C) $(DEC_H)
	black -q -l 80 scripts/*.py

check:
	clang-format -n -Werror -style=file \
		$(LIB_C) $(LIB_H) $(ENC_C) $(ENC_H) $(DEC_C) $(DEC_H)
	clang-tidy --warnings-as-errors --config= \
		$(LIB_C) $(LIB_H) $(ENC_C) $(ENC_H) $(DEC_C) $(DEC_H) -- $(INC)
	black -q -l 80 --check scripts/*.py

clean:
	rm -r dist

.PHONY: format check clean
