#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
  char *p;
  int *ip;
  p = malloc(4096); // allocate one page
  if(p == 0){
    printf("malloc failed\n");
    exit(1);
  }
  ip = (int*)p;
  *ip = 42; // initial value

  int pid = fork();
  if(pid < 0){
    printf("fork failed\n");
    exit(1);
  } else if(pid == 0){
    printf("child: before write, value = %d\n", *ip);
    *ip = 99; // should cause COW and not affect parent
    printf("child: after write, value = %d\n", *ip);
    exit(0);
  } else {
    wait(0);
    printf("parent: after child exit, value = %d\n", *ip);
    exit(0);
  }
}
