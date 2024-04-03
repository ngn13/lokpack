#pragma once

extern char *FTP_URL;
extern char *FTP_USER;
extern char *FTP_PWD;
extern char *EXT;

void upload_file(void *);
void encrypt_file(void *);
