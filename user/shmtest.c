#include "kernel/types.h"
#include "user/user.h"

int main(void)
{
  printf("=== Step by Step SHM Debug Test ===\n");

  // Step 1: Test shm_create
  printf("Step 1: Testing shm_create\n");
  int key = 123;
  int idx = shm_create(key);
  printf("shm_create(%d) returned: %d\n", key, idx);
  if (idx < 0)
  {
    printf("ERROR: shm_create failed\n");
    exit(1);
  }

  // Step 2: Test shm_get
  printf("\nStep 2: Testing shm_get\n");
  uint64 addr_val = shm_get(key);
  printf("shm_get(%d) returned: 0x%lx\n", key, addr_val);
  if (addr_val == 0)
  {
    printf("ERROR: shm_get failed\n");
    exit(1);
  }

  // Step 3: Test basic pointer access
  printf("\nStep 3: Testing pointer conversion\n");
  char *ptr = (char *)addr_val;
  printf("Converted to char pointer: %p\n", ptr);

  // Step 4: Test read (should be zero from memset)
  printf("\nStep 4: Testing read from shared memory\n");
  printf("Reading byte 0 (should be 0): %d\n", (int)ptr[0]);

  // Step 5: Test write
  printf("\nStep 5: Testing write to shared memory\n");
  printf("Writing 'T' to byte 0...\n");
  ptr[0] = 'T';
  printf("Write completed\n");

  // Step 6: Test read-back
  printf("\nStep 6: Testing read-back\n");
  printf("Reading back byte 0: '%c' (should be 'T')\n", ptr[0]);

  // Step 7: Test multiple bytes
  printf("\nStep 7: Testing multiple byte access\n");
  for (int i = 0; i < 10; i++)
  {
    ptr[i] = 'A' + i;
  }
  printf("Wrote A-J to first 10 bytes\n");

  printf("Reading back: ");
  for (int i = 0; i < 10; i++)
  {
    printf("%c", ptr[i]);
  }
  printf("\n");

  // Step 8: Clean up
  printf("\nStep 8: Cleaning up\n");
  int result = shm_close(key);
  printf("shm_close(%d) returned: %d\n", key, result);

  printf("\n=== All tests completed successfully! ===\n");
  exit(0);
}