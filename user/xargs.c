#include "kernel/param.h"
#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  char *cmd_argv[MAXARG];
  int cmd_argc = 0;
  for (int i = 1; i < argc; i++) {
    cmd_argv[cmd_argc] = malloc(strlen(argv[i]) + 1);
    strcpy(cmd_argv[cmd_argc], argv[i]);
    cmd_argc++;
  }
  char c[2];
  char cmd_buf[512];
  while (read(0, c, 1) == 1) {
    if (strcmp(c, "\n") == 0) {
      cmd_argv[cmd_argc] = malloc(strlen(cmd_buf) + 1);
      strcpy(cmd_argv[cmd_argc], cmd_buf);
      if (fork() == 0) {
        exec(cmd_argv[0], cmd_argv);
      }
      wait(0);
      memset(cmd_buf, 0, 512);
    } else {
      char *p;
      p = cmd_buf + strlen(cmd_buf);
      *p = c[0];
    }
  }
  exit(0);
}
