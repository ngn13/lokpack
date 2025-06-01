# commands & flags
CC = gcc

ifneq (, $(shell which clang-format))
  FORMATTER = clang-format
else
  FORMATTER = clang-format-19
endif

ifneq (, $(shell which clang-tidy))
  LINTER = clang-tidy
else
  LINTER = clang-tidy-19
endif

CFLAGS  = -Wall -Wextra -Werror -std=gnu89 -pedantic
CFLAGS += -Wno-overlength-strings # public/private key is larger than 509 bytes
CFLAGS += -fstack-protector-strong -fstack-clash-protection
CFLAGS += -z noexecstack -Wa,--noexecstack

INC  = -I./inc
LIBS = -lpthread -lcurl -lssl -lcrypto

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
LP_PUBKEY  = $(shell bash scripts/nonl.sh keys/public )
LP_PRIVKEY = $(shell bash scripts/nonl.sh keys/private)

all: dist/encryptor dist/decryptor

dist/encryptor: $(ENC_C) $(ENC_H) $(LIB_C) $(LIB_H)
	@mkdir -pv dist/
	$(CC) $(EXTRAFLAGS) $(CFLAGS) $(INC)                    \
		-DLP_DEBUG=${LP_DEBUG} -DLP_VERSION=\"${LP_VERSION}\" \
		-DLP_PUBKEY=\""${LP_PUBKEY}"\" -ULP_PRIVKEY           \
		$(ENC_C) $(LIB_C) -o $@ $(LIBS)

dist/decryptor: $(DEC_C) $(DEC_H) $(LIB_C) $(LIB_H)
	@mkdir -pv dist/
	$(CC) $(EXTRAFLAGS) $(CFLAGS) $(INC)                              \
		-DLP_DEBUG=${LP_DEBUG} -DLP_VERSION=\"${LP_VERSION}\"           \
		-DLP_PUBKEY=\""${LP_PUBKEY}"\" -DLP_PRIVKEY=\""${LP_PRIVKEY}"\" \
		-DLP_THREADS=${LP_THREADS}                                      \
		$(DEC_C) $(LIB_C) -o $@ $(LIBS)

format:
	$(FORMATTER) -i -Werror -style=file \
		$(LIB_C) $(LIB_H) $(ENC_C) $(ENC_H) $(DEC_C) $(DEC_H)
	black -q -l 80 scripts/*.py

check:
	$(FORMATTER) -n -Werror -style=file \
		$(LIB_C) $(LIB_H) $(ENC_C) $(ENC_H) $(DEC_C) $(DEC_H)
	$(LINTER) --warnings-as-errors --config= \
		$(LIB_C) $(LIB_H) $(ENC_C) $(ENC_H) $(DEC_C) $(DEC_H) -- $(INC)
	black -q -l 80 --check scripts/*.py

test:
	bash ./scripts/test.sh

clean:
	rm -f dist/encryptor
	rm -f dist/decryptor

.PHONY: format check test clean
