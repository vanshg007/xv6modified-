#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGBLOCKS    (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       4000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
#define USERSTACK    1     // user stack pages

// kernel/param.h  (or put near other limits)
#define MAXSHM        64      // max distinct shared memory keys
#define MAXSHM_PERP   16      // max shared pages a process can attach
#define MBOX_MAX      64      // number of mailboxes
#define MBOX_CAP      32      // messages per mailbox (bounded buffer)
#define PGSIZE 4096  // bytes per page

