#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

int main() {
  int input_pipe[2];
  pipe(input_pipe);
  if (fork() == 0) {
    close(input_pipe[1]);
    int create_child = 0;
    int num;
    int first_num;
    int get_first_num = 0;
    int curr_pid = getpid();
    while (1) {
      num = 0;
      int n = read(input_pipe[0], &num, 4);
      if (n == 0) {
        // input finished;
        close(input_pipe[1]);
        wait(0);
        exit(0);
      }
      if (n != 4) {
        printf("read error\n");
        exit(1);
      }
      if (!get_first_num) {
        first_num = num;
        get_first_num = 1;
        printf("prime %d\n", first_num);
      } else {
        if (!create_child) {
          int tem_pipe[2];
          pipe(tem_pipe);
          if (fork() == 0) {
            get_first_num = 0;
            close(input_pipe[0]);
            input_pipe[0] = tem_pipe[0];
            input_pipe[1] = tem_pipe[1];
            close(input_pipe[1]);
            curr_pid = getpid();
          } else {
            create_child = 1;
            input_pipe[1] = tem_pipe[1];
            close(tem_pipe[0]);
          }
        }
        if (create_child && (num % first_num != 0)) {
          int w_n = write(input_pipe[1], &num, 4);
          if (w_n != 4) {
            printf("%d write error\n", curr_pid);
            exit(1);
          }
        }
      }
    }
  } else {
    int start_num = 2;
    int end_num = 35;
    close(input_pipe[0]);
    for (int i = start_num; i <= end_num; i++) {
      int w_n = write(input_pipe[1], &i, 4);
      if (w_n != 4) {
        printf("write input error");
        exit(1);
      }
    }
    close(input_pipe[1]);
    wait(0);
  }
  exit(0);
}
