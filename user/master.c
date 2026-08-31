#include "kernel/types.h"
#include "user/user.h"

#define SHM_KEY 1
#define MBOX_A2B 100
#define MBOX_B2A 200
#define END_MARKER 0xFFFF

int
main(int argc, char *argv[]){
  int shmid = shm_create(SHM_KEY);
  if(shmid < 0){
    printf("master: shm_create failed\n");
    exit(1);
  }
  int *maze = (int*) shm_get(SHM_KEY);
  if(maze == 0){
    printf("master: shm_get failed\n");
    exit(1);
  }

  maze[0] = 2;
  maze[1] = 3;
  maze[2] = 4;
  maze[3] = 5;
  maze[4] = END_MARKER;
  maze[5] = END_MARKER;

  int a2b = mbox_create(MBOX_A2B);
  int b2a = mbox_create(MBOX_B2A);

  if(a2b < 0 || b2a < 0){
    printf("master: mbox_create failed\n");
    exit(1);
  }

  if(fork() == 0){
    // child = Process A (send-first)
    char *args[] = {"process", "A", "0", "sendfirst", 0};
    exec("process", args);
    printf("master: exec process A failed\n");
    exit(1);
  }

  if(fork() == 0){
    // child = Process B (recv-first)
    char *args[] = {"process", "B", "1", "recvfirst", 0};
    exec("process", args);
    printf("master: exec process B failed\n");
    exit(1);
  }
  wait(0);
  wait(0);

  printf("master: both processes finished\n");
  exit(0);
}
