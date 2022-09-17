#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

int main(void) {
  int p_to_child[2];
  int p_to_parent[2];
  char buf[10];
  pipe(p_to_child);
  pipe(p_to_parent);
  if (fork() == 0) {
    close(p_to_child[1]);
    close(p_to_parent[0]);
    int read_num = read(p_to_child[0], buf, 1);
    if (read_num == 0) {
      printf("read pipeline closed, read nothing\n");
      exit(1);
    }
    if (read_num < 0) {
      printf("failed to read\n");
      exit(1);
    }
    int self_pid = getpid();
    printf("%d: received ping\n", self_pid);
    close(p_to_child[0]);
    if (write(p_to_parent[1], buf, 1) != 1) {
      printf("write to parent error\n");
      exit(1);
    }
    close(p_to_parent[1]);
    exit(0);
  } else {
    close(p_to_child[0]);
    close(p_to_parent[1]);
    if (write(p_to_child[1], "a", 1) != 1) {
      printf("write to child error\n");
      exit(1);
    }
    close(p_to_child[1]);
    int read_num = read(p_to_parent[0], buf, 1);
    if (read_num == 0) {
      printf("parent: read pipeline closed, read nothing\n");
      exit(1);
    }
    if (read_num < 0) {
      printf("parent: failed to read\n");
      exit(1);
    }
    int self_pid = getpid();
    printf("%d: received pong\n", self_pid);
    close(p_to_parent[0]);
    exit(0);
  }
}
