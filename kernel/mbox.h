// kernel/mbox.h
#ifndef MBOX_H
#define MBOX_H

#define MAX_MBOX 32       // maximum mailboxes
#define MB_MAX_MSG 16     // capacity per mailbox

void mboxinit(void);

int mbox_create(int key);
int mbox_send(int mbox_id, int msg);
int mbox_recv(int mbox_id, int *msg);

#endif
