#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define NUMSEMS 3
#define RES_FREE 2

typedef struct {
    int a, b;
    long result;
    pid_t client;
} mem_t;

key_t key;
int shmid;
int semid;

mem_t* data;
struct sembuf free_resource[1] = {{RES_FREE, 1, 0}};
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
    signal(SIGINT, handler);
}

int main()
{
    key = ftok("/bin/ls", '1');
    shmid = shmget(key, sizeof(mem_t), IPC_CREAT | IPC_EXCL | 0600);
    semid = semget(key, NUMSEMS, IPC_CREAT | IPC_EXCL | 0600);
    if ((shmid == -1) || (semid == -1)) {
        printf("ipc get failed\n");
        return 1;
    }

    data = (mem_t*)shmat(shmid, NULL, 0);
    if (data == (mem_t*)(-1)) {
        printf("shmat failed\n");
        return 2;
    }

    signal(SIGINT, handler);
    semop(semid, free_resource, 1);

    while (1) {
        semop(semid, wait_client, 1);
        data->result = data->a + data->b;
        printf("in process %d + %d = %ld, client = %d\n",
                data->a, data->b, data->result, (int)data->client);
        fflush(stdout);
        semop(semid, calculated, 1);       
    }
    return 0;
}
