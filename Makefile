# commands/tools
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

# compile time options
LP_DEBUG     = 0
LP_VERSION   = $(shell cat scripts/common.sh | grep VERSION | cut -d "=" -f2)
LP_THREADS   = 20
LP_QUEUE_MAX = 200
LP_PUBKEY    = $(shell bash scripts/nonl.sh keys/public )
LP_PRIVKEY   = $(shell bash scripts/nonl.sh keys/private)

# flags
CFLAGS  = -Wall -Wextra -Werror -std=gnu89 -pedantic
CFLAGS += -Wno-overlength-strings # public/private key is larger than 509 bytes
CFLAGS += -fstack-protector-strong -fstack-clash-protection
CFLAGS += -z noexecstack -Wa,--noexecstack

INC  = -I./inc
LIBS = -lpthread -lcurl -lssl -lcrypto

OPTS  = -DLP_DEBUG=${LP_DEBUG} -DLP_VERSION=\"${LP_VERSION}\"
OPTS += -DLP_PUBKEY=\""${LP_PUBKEY}"\" -DLP_PRIVKEY=\""${LP_PRIVKEY}"\"
OPTS += -DLP_QUEUE_MAX=${LP_QUEUE_MAX} -DLP_THREADS=${LP_THREADS}

# sources
LIB_C = $(wildcard src/lib/*.c)
LIB_H = $(wildcard inc/lib/*.h)

ENC_C  = $(wildcard src/encryptor/*.c)
ENC_H  = $(wildcard inc/encryptor/*.h)
ENC_O  = $(patsubst src/encryptor/%.c,dist/%.enc.o,$(ENC_C))
ENC_O += $(patsubst src/lib/%.c,dist/%.enc.o,$(LIB_C))

DEC_C  = $(wildcard src/decryptor/*.c)
DEC_H  = $(wildcard inc/decryptor/*.h)
DEC_O  = $(patsubst src/decryptor/%.c,dist/%.dec.o,$(DEC_C))
DEC_O += $(patsubst src/lib/%.c,dist/%.dec.o,$(LIB_C))

all: dist/encryptor dist/decryptor

help:
	@echo "lokpack $(LP_VERSION) - here are all the build options"
	@echo
	@echo "LP_DEBUG    : enable debug messages"
	@echo "LP_THREADS  : thread count for the decryptor (default is $(LP_THREADS))"
	@echo "LP_QUEUE_MAX: max queue length for the thread pool (default is $(LP_QUEUE_MAX))"
	@echo "LP_PUBKEY   : RSA public key (by default read from keys/public)"
	@echo "LP_PRIVKEY  : RSA private key (by default read from keys/private)"
	@echo
	@echo "don't do crime!"

dist/encryptor: $(ENC_O) $(ENC_H) $(LIB_H)
	@mkdir -pv dist/
	$(CC) $(EXTRAFLAGS) $(CFLAGS) $(INC) $(ENC_O) -o $@ $(LIBS)

dist/decryptor: $(DEC_O) $(DEC_H) $(LIB_H)
	@mkdir -pv dist/
	$(CC) $(EXTRAFLAGS) $(CFLAGS) $(INC) $(DEC_O) -o $@ $(LIBS)

# encryptor object files
dist/%.enc.o: src/encryptor/%.c $(ENC_H) $(LIB_H)
	@$(CC) -c $(EXTRAFLAGS) $(CFLAGS) $(INC) $(OPTS) $< -o $@

dist/%.enc.o: src/lib/%.c $(ENC_H) $(LIB_H)
	@$(CC) -c $(EXTRAFLAGS) $(CFLAGS) $(INC) $(OPTS) -DLP_ENCRYPTOR $< -o $@

# decryptor object files
dist/%.dec.o: src/decryptor/%.c $(DEC_H) $(LIB_H)
	@$(CC) -c $(EXTRAFLAGS) $(CFLAGS) $(INC) $(OPTS) $< -o $@

dist/%.dec.o: src/lib/%.c $(DEC_H) $(LIB_H)
	@$(CC) -c $(EXTRAFLAGS) $(CFLAGS) $(INC) $(OPTS) -DLP_DECRYPTOR $< -o $@

format:
	$(FORMATTER) -i -Werror -style=file \
		$(LIB_C) $(LIB_H) $(ENC_C) $(ENC_H) $(DEC_C) $(DEC_H)
	black -q -l 80 scripts/*.py

check:
	@echo "checking formatting errors"
	@$(FORMATTER) -n -Werror -style=file \
		$(LIB_C) $(LIB_H) $(ENC_C) $(ENC_H) $(DEC_C) $(DEC_H)
	@black -q -l 80 --check scripts/*.py
	@echo "checking for linting errors"
	@$(LINTER) --warnings-as-errors --config= \
		$(LIB_C) $(LIB_H) $(ENC_C) $(ENC_H) $(DEC_C) $(DEC_H) -- \
		$(INC) $(OPTS) -DLP_ENCRYPTOR

test:
	bash ./scripts/test.sh

clean:
	rm -f dist/encryptor
	rm -f dist/decryptor
	rm -f dist/*.o

.PHONY: format check test clean
