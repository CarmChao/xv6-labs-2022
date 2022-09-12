#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void findfile(int path_num, char *path[10], char* file) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  for (int i=0; i<path_num; i++) {
    if ((fd = open(path[i], 0)) < 0) {
      fprintf(1, "ls folder %s failed\n", path[i]);
      return;
    }
    strcpy(buf, path[i]);
    p = buf + strlen(buf);
    *p++ = '/';
    char *tmp_ptr[10];
    int tmp_path_num = 0;
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0) {
        continue;
      }
      if (strcmp(de.name, ".")==0 || strcmp(de.name, "..")==0) {
        continue;
      }
      memmove(p, de.name, DIRSIZ);
      if (stat(buf, &st) < 0) {
        printf("cant get %s stat", buf);
        continue;
      }
      if (st.type == T_DIR) {
        tmp_ptr[tmp_path_num] = malloc(strlen(buf) + 1);
        strcpy(tmp_ptr[tmp_path_num], buf);
        tmp_path_num += 1;
      }
      else if (strcmp(de.name, file) == 0) {
        printf("%s\n", buf);
      }
    }
    findfile(tmp_path_num, tmp_ptr, file);
  }
  return;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: find <dir> <files>");
    exit(1);
  }
  char *path[10];
  path[0] = argv[1];
  findfile(1, path, argv[2]);
  exit(0);
}
