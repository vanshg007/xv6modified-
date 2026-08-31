// user/mrumem.c
#include "kernel/types.h"
#include "user/user.h"

#define PAGES 512       // number of pages to allocate; tune relative to RAM
#define TOUCHS 10000    // number of random accesses
#define PGSIZE 4096

int
main(int argc, char *argv[])
{
  int i;
  char *mem = sbrk(PAGES * PGSIZE);
  if(mem == (char*)-1){
    printf("sbrk failed\n");
    exit(1);
  }

  // initial touches to create pages
  for(i = 0; i < PAGES; i++){
    char *p = mem + i * PGSIZE;
    p[0] = (char)(i & 0xff);
  }

  // Randomly touch pages to create MRU churn
  int seed = 12345;
  for(i = 0; i < TOUCHS; i++){
    seed = (seed * 1103515245 + 12345);
    int idx = (seed >> 16) % PAGES;
    char *p = mem + idx * PGSIZE;
    p[0] = (char)((p[0] + 1) & 0xff);

    if(i % 500 == 0){
      struct pagestat st;
      if(getpagestat(getpid(), &st) == 0){
        printf("iter %d PF %d SI %d SO %d\n", i, st.page_faults, st.swap_ins, st.swap_outs);
      } else {
        printf("getpagestat fail\n");
      }
      dumpmru();
    }
  }

  printf("done\n");
  exit(0);
}
