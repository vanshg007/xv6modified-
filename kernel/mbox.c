#include "types.h"
#include "riscv.h"      
#include "spinlock.h"
#include "defs.h"
#include "proc.h"

#define MAX_MBOX 32
#define MBOX_SIZE 16

struct mailbox {
 int key;
 int buf[MBOX_SIZE];
 int head, tail, count;
 struct spinlock lock;
};

struct {
 struct mailbox box[MAX_MBOX];
} mbox;

void
mboxinit(void){
 for(int i=0;i<MAX_MBOX;i++){
   mbox.box[i].key=-1;
   initlock(&mbox.box[i].lock,"mbox");
 }
}

int
mbox_create(int key){
 for(int i=0;i<MAX_MBOX;i++){
   acquire(&mbox.box[i].lock);
   if(mbox.box[i].key==key){
     release(&mbox.box[i].lock);
     return i;
   }
   if(mbox.box[i].key==-1){
     mbox.box[i].key=key;
     mbox.box[i].head=mbox.box[i].tail=mbox.box[i].count=0;
     release(&mbox.box[i].lock);
     return i;
   }
   release(&mbox.box[i].lock);
 }
 return -1;
}

int
mbox_send(int id,int msg){
 if(id<0||id>=MAX_MBOX) return -1;
 struct mailbox *m=&mbox.box[id];
 acquire(&m->lock);
 while(m->count==MBOX_SIZE){
   sleep(m,&m->lock);
 }
 m->buf[m->tail]=msg;
 m->tail=(m->tail+1)%MBOX_SIZE;
 m->count++;
 wakeup(m);
 release(&m->lock);
 return 0;
}
int
mbox_recv(int id,int *msg){
 if(id<0||id>=MAX_MBOX) return -1;
 struct mailbox *m=&mbox.box[id];
 acquire(&m->lock);
 while(m->count==0){
   sleep(m,&m->lock);
 }
 *msg=m->buf[m->head];
 m->head=(m->head+1)%MBOX_SIZE;
 m->count--;
 wakeup(m);
 release(&m->lock);
 return 0;
}