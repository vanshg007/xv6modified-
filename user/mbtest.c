#include "kernel/types.h"
#include "user/user.h"

// Simple busy-wait sleep (not a syscall)
void my_sleep(int ticks)
{
  for (volatile int i = 0; i < 100000 * ticks; i++)
  {
    // do nothing, just burn CPU cycles
  }
}



int main(void)
{
  int key = 7;
  int id = mbox_create(key);
  if (id < 0)
  {
    printf("Mailbox creation failed!\n");
    exit(1);
  }

  if (fork() == 0)
  {
    // Child receives
    for (int i = 0; i < 5; i++)
    {
      int v = -1;
      if (mbox_recv(id, &v) == 0)
      {
        printf("Child got %d\n", v);
      }
      else
      {
        printf("Child recv error\n");
      }
    }
    exit(0);
  }
  else
  {
    // Parent sends
    for (int i = 0; i < 5; i++)
    {
      printf("Parent sending %d\n", i);
      if (mbox_send(id, i) < 0)
      {
        printf("Send error!\n");
      }
      my_sleep(10);
    }
    wait(0);
  }
  exit(0);
}