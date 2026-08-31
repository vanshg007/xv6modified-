#include "kernel/types.h"
#include "user/user.h"

#define SHM_KEY 1
#define MBOX_A2B_KEY 100
#define MBOX_B2A_KEY 200
#define END_MARKER 0xFFFF
#define DONE_MSG (-1)

struct lock
{
    volatile int busy;
};

void lock_acquire(struct lock *l)
{
    while (__sync_lock_test_and_set(&l->busy, 1) != 0)
        ;
}

void lock_release(struct lock *l)
{
    __sync_lock_release(&l->busy);
}

void my_sleep(int ticks)
{
    for (volatile int i = 0; i < 20000 * ticks; i++)
    {
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("usage: process <name> <start_index> <role>\n");
        exit(1);
    }

    char *name = argv[1];
    int pos = atoi(argv[2]);
    int sendfirst = strcmp(argv[3], "sendfirst") == 0;
    int *shm = (int *)shm_get(SHM_KEY);
    if (shm == 0)
    {
        printf("process %s: shm_get failed\n", name);
        exit(1);
    }
    // maze starts at shm[0], lock at shm[32] (avoid overlap)
    int *maze = shm;
    struct lock *print_lock = (struct lock *)&shm[32];

    int mbox_a2b = mbox_create(MBOX_A2B_KEY);
    int mbox_b2a = mbox_create(MBOX_B2A_KEY);
    if (mbox_a2b < 0 || mbox_b2a < 0)
    {
        printf("process %s: mbox_create failed (a2b=%d b2a=%d)\n", name, mbox_a2b, mbox_b2a);
        exit(1);
    }

    int send_id = (strcmp(name, "A") == 0) ? mbox_a2b : mbox_b2a;
    int recv_id = (strcmp(name, "A") == 0) ? mbox_b2a : mbox_a2b;

    int partner_pos = -1;
    int step = 0;
    int my_done = 0, partner_done = 0;

    while (1)
    {
        if (!my_done && !sendfirst)
        {
            if (mbox_recv(recv_id, &partner_pos) < 0)
            {
                lock_acquire(print_lock);
                printf("process %s: mbox_recv failed\n", name);
                lock_release(print_lock);
                exit(1);
            }
            if (partner_pos == DONE_MSG)
                partner_done = 1;
        }

        if (!my_done)
        {
            int out = pos;
            if (mbox_send(send_id, out) < 0)
            {
                lock_acquire(print_lock);
                printf("process %s: mbox_send failed\n", name);
                lock_release(print_lock);
                exit(1);
            }
        }
        else
        {
            if (!partner_done)
            {
                if (mbox_send(send_id, DONE_MSG) < 0)
                {
                    lock_acquire(print_lock);
                    printf("process %s: mbox_send(DONE) failed\n", name);
                    lock_release(print_lock);
                    exit(1);
                }
            }
        }

        if (!my_done && sendfirst)
        {
            if (mbox_recv(recv_id, &partner_pos) < 0)
            {
                lock_acquire(print_lock);
                printf("process %s: mbox_recv failed\n", name);
                lock_release(print_lock);
                exit(1);
            }
            if (partner_pos == DONE_MSG)
                partner_done = 1;
        }

        if (!my_done && partner_pos >= 0)
        {
            int next = maze[partner_pos];

            lock_acquire(print_lock);
            printf("process %s step=%d partner_at=%d -> my_next=%d\n",
                   name, step, partner_pos, next);
            lock_release(print_lock);

            if (next == END_MARKER)
            {
                my_done = 1;
                lock_acquire(print_lock);
                printf("process %s: detected partner_done=%d, my_done=%d, breaking loop\n",
                       name, partner_done, my_done);
                lock_release(print_lock);
                break;
            }
            else
            {
                pos = next;
            }
            step++;
        }

        my_sleep(1);
    }

    lock_acquire(print_lock);
    printf("process %s: both done, exiting\n", name);
    lock_release(print_lock);

    exit(0);
}
