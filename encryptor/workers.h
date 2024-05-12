#pragma once

typedef struct upload_args {
  char *user;
  char *pwd;
  char *file;
  char *url;
} upload_args_t;

typedef struct encrypt_args {
  struct stat *st;
  char        *file;
  char        *ext;
} encrypt_args_t;

void upload_file(void *);
void encrypt_file(void *);
