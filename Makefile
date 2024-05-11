LIB_C = $(wildcard lib/*.c)
LIB_H = $(wildcard lib/*.h)

ENC_C = $(wildcard encryptor/*.c) $(wildcard encryptor/*/*.c)
ENC_H = $(wildcard encryptor/*.h encryptor/*/*.h)

DEC_C = $(wildcard decryptor/*.c) $(wildcard decryptor/*/*.c)
DEC_H = $(wildcard decryptor/*.h decryptor/*/*.h)

CC = gcc

BUILD_PUB = "-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvwdgrC8YKf+nVkt8607I\n2yx5iPL3oNL64aBa+R1sRivUjn7WP8fuhMFGC2409zWQD5ZkPNdgK9F+rFyakj7U\n+WfGVjsv+DebmoN9EgLFF4rLSr7AHXt+MCRDWr3mrifA8a7mmtNC+dGh3YNQxFZp\nyTvOxThDM6hXy7h+TePR1NY7TDtZa901YRhWgXqm++ISw2NZqRzQxbBsdbVcNhMe\nIHRxZuYaf2V4n1jINkMtWpx7d5/TDEENmOr8XixTyUY6/qCsJ8/0vCq/5f1A++5u\nN/s/xjC+bxjuw7vQHaXS19q9sKhqXM8gwH7tyF8VoFaFxsvnXYsXDeCve1sgLHEI\n7wIDAQAB\n-----END PUBLIC KEY-----\n"
BUILD_PRIV = "-----BEGIN PRIVATE KEY-----\nMIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQC/B2CsLxgp/6dW\nS3zrTsjbLHmI8veg0vrhoFr5HWxGK9SOftY/x+6EwUYLbjT3NZAPlmQ812Ar0X6s\nXJqSPtT5Z8ZWOy/4N5uag30SAsUXistKvsAde34wJENaveauJ8Dxruaa00L50aHd\ng1DEVmnJO87FOEMzqFfLuH5N49HU1jtMO1lr3TVhGFaBeqb74hLDY1mpHNDFsGx1\ntVw2Ex4gdHFm5hp/ZXifWMg2Qy1anHt3n9MMQQ2Y6vxeLFPJRjr+oKwnz/S8Kr/l\n/UD77m43+z/GML5vGO7Du9AdpdLX2r2wqGpczyDAfu3IXxWgVoXGy+ddixcN4K97\nWyAscQjvAgMBAAECggEAC0H2+bonWMLZTKgcpE0bhLw/vsAEjpuL1CiTA+qcexwt\nJdTvTK3zJStFRx6+MHsdZMygxFlq+ZD1rX0iLNkyXlN/Oo1LG+nCdUveTIo2P7La\n5k7VpNlvnh7T1HoZhJ81j8QWR7WtHkX8VIoV8e1MV2L7ayi0u3AdPie+wTURNq7H\nBzajWMsyUv1WDHAAXWOlD29GnKmHFKBuFtVlFVmxFpLf3ZkqAODWz26DLmA4rVKO\ntHUcguwP8qEvG3+erp+958f6XLs2paxGFGo+edMcED4vsLJcXdmSt7JCZTYe5V/d\n/Sf9gVdgggaEAr30LM4UjEQsQ1Hj4lU0BbfA4LsxAQKBgQDgbHgsH3v8d+S81NIQ\n34duiBxb1tT6Y5NIZE4+Jv1dzPHxQVTbfHLVlYi/p2NAOZbYI7GJs7qB7u/5YJOT\nG+ApFvJjgEdzM69PFxPWia1u+6M9TWBsutGvwPeEzZadxI36QGndRaEhYsurBoX+\nMKOul1Gl1aoHwS5PxmH3srHnAQKBgQDZ6A5EYQAdQ2Ng5OpcpU3VUYfq5GZarw3C\nItALZe0YZ1v9Q0sd0dtIgE0IB89ncAQdzKAzw38/sz3xKGVWiXk4F5LLkGoPx5ul\noz4pUYhgk+H0RrDXA6M0dDPbYs/PTXSVxIxrIa3eK1jbhxcdTd4lwC7Qc7usBpPZ\nvoraDKFf7wKBgCCoiEBbgI534Ah+q+casDmsgvv2Yl3hmbBCMISo5dv9lAO4ryGV\nlZxxO0owkKfq+YGQiChyyCRaYf+XR0J0HwW7uiAyrbMdBVjsEc35kQyCf/nOxcZs\n1VDPMvXXeSCSVtrg/sbdeBpEp0DHHT6lU1x31sDcO/cO7K9dPipujacBAoGAdj3P\nkBc6oqZwhKFy8rbYV0qYgENTrx2ST6foUGcMktNzPSiI2DOJ0/ua0SK7zpsgALOK\nsswAfqZalEOhSaXrWx6fj0pb4xgjQ2wjTPRrFkzorU62otXcd5gek4s4zHpxKtuu\nfmuWFxpaazUQu9wHEWEJhh9m1N/01pDoc42q+40CgYBRPze/XWyGw85aekGGkNb+\nlf3JQ5+Krx7ckR6yiUNrEcoHx1cwBykkZeHdY6nM6M65b5NcJ1aHib5dlFLs3eVr\nrUqgSucwdHtUpjkLlg6uqhaFFFl4/s+uTOtzGcaRKVZ5dIyAq/zOyvJr5NUqvRNg\na3izteWTKlKtd2FQBjgbNA==\n-----END PRIVATE KEY-----\n"
VERSION = $(shell cat scripts/common.sh | grep VERSION | cut -d "=" -f2)
DEBUG = false

all: dist/encryptor dist/decryptor

dist/encryptor: $(ENC_C) $(ENC_H) $(LIB_C) $(LIB_H)
	mkdir -pv dist/
ifeq ($(findstring mingw,$(CC)),mingw)
		$(CC) $(CFLAGS) $(ENC_C) $(LIB_C) -DVERSION=\"${VERSION}\" -DBUILD_PUB=\"${BUILD_PUB}\" -o $@ -lcurl -lcrypto
else
		$(CC) $(CFLAGS) $(ENC_C) $(LIB_C) -DVERSION=\"${VERSION}\" -DBUILD_PUB=\"${BUILD_PUB}\" -o $@ -lpthread -lcurl -lcrypto
endif

dist/decryptor: $(DEC_C) $(DEC_H) $(LIB_C) $(LIB_H)
	mkdir -pv dist/
ifeq ($(findstring mingw,$(CC)),mingw)
		$(CC) $(CFLAGS) $(DEC_C) $(LIB_C) -DDEBUG_MODE=${DEBUG} -DVERSION=\"${VERSION}\" -DBUILD_PUB=\"${BUILD_PUB}\" -DBUILD_PRIV=\"${BUILD_PRIV}\" -o $@ -lcrypto
else
		$(CC) $(CFLAGS) $(DEC_C) $(LIB_C) -DDEBUG_MODE=${DEBUG} -DVERSION=\"${VERSION}\" -DBUILD_PUB=\"${BUILD_PUB}\" -DBUILD_PRIV=\"${BUILD_PRIV}\" -o $@ -lpthread -lcrypto
endif

format:
	clang-format -i -style=file ./*/*.c ./*/*.h

.PHONY: format
