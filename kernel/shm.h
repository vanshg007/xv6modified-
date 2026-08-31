// kernel/shm.h
#ifndef SHM_H
#define SHM_H

#define MAXSHM 64
#define SHM_KEY_MAX 128

void shminit(void);
int shm_create(int key);
uint64 shm_get(int key);
int shm_close(int key);
//void shm_proc_cleanup(struct proc *p);

#endif
