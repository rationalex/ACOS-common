#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define NUMSEMS 2
#define SHMEMPATH "/bin/ls"
#define SHMEMKEY '1'

typedef struct {
    int a, b;
    long result;
    int flg;
} mem_t;

key_t key;
int shmid;
int semid;

mem_t* data;
struct sembuf sb[2] = {{0, 1, 0}, {0, 0, 0}};
struct sembuf wait_client[1] = {{0, -1, 0}};
struct sembuf calculated[1] = {{1, 1, 0}};

void handler(int sig)
{
    if (sig == SIGINT)
    {
        shmdt(data);
        semctl(semid, NUMSEMS, IPC_RMID);
        shmctl(shmid, IPC_RMID, NULL);
        exit(0);
    }
    signal(SIGUSR1, handler);
}

int main()
{
    key = ftok(SHMEMPATH, SHMEMKEY);
    shmid = shmget(key, sizeof(mem_t), IPC_CREAT | 0600);
    if (shmid == -1)
    {
        perror("shared memory id get error");
        return 1;
    }

    semid = semget(key, NUMSEMS, IPC_CREAT | 0600);
    if (semid == -1)
    {
        printf("semaphors id get failed\n");
        return 1;
    }

    data = (mem_t*)shmat(shmid, NULL, 0);
    if (data == (mem_t*)(-1))
    {
        printf("shmat failed\n");
        return 2;
    }

    signal(SIGINT, handler); //we need some way to exit infinite loop

    while (1)
    {
      semop(semid, wait_client, 1);
      data->result = data->a + data->b;
      semop(semid, calculated, 1);
    }

    return 0;
}
