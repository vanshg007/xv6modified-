#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define BIGFILE "bigfile"
#define CHUNK_SIZE 512
#define TOTAL_SIZE (MAXFILE * BSIZE)   // try writing up to maximum file size supported

#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200
#define O_TRUNC   0x400

int
main(void)
{
  int fd, i;
  char wbuf[CHUNK_SIZE];
  char rbuf[CHUNK_SIZE];
  int bytes_written = 0, bytes_read = 0;

  printf("bigfile: creating large file %s\n", BIGFILE);
  fd = open(BIGFILE, O_CREATE | O_RDWR);
  if(fd < 0){
    printf("bigfile: cannot create %s\n", BIGFILE);
    exit(1);
  }

  // fill write buffer with a known pattern
  for(i = 0; i < CHUNK_SIZE; i++)
    wbuf[i] = i % 128;

  // write repeatedly until we exceed the old limit (~268KB)
  for(i = 0; i < TOTAL_SIZE / CHUNK_SIZE; i++){
    if(write(fd, wbuf, CHUNK_SIZE) != CHUNK_SIZE){
      printf("bigfile: write failed at block %d\n", i);
      break;
    }
    bytes_written += CHUNK_SIZE;

    if(i % 200 == 0)
      printf("  wrote %d KB...\n", bytes_written / 1024);
  }

  printf("bigfile: total bytes written = %d\n", bytes_written);
  close(fd);

  // reopen and verify
  fd = open(BIGFILE, O_RDONLY);
  if(fd < 0){
    printf("bigfile: cannot reopen %s\n", BIGFILE);
    exit(1);
  }

  for(i = 0; ; i++){
    int n = read(fd, rbuf, CHUNK_SIZE);
    if(n == 0)
      break;
    if(n < 0){
      printf("bigfile: read error\n");
      exit(1);
    }
    for(int j = 0; j < n; j++){
      if(rbuf[j] != (j % 128)){
        printf("bigfile: data mismatch at block %d offset %d\n", i, j);
        exit(1);
      }
    }
    bytes_read += n;
    if(i % 200 == 0)
      printf("  verified %d KB...\n", bytes_read / 1024);
  }

  close(fd);
  printf("bigfile: successfully verified %d bytes!\n", bytes_read);
  printf("bigfile: test passed\n");
  exit(0);
}
