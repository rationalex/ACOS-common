#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

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
struct sembuf use_resource[1] = {{RES_FREE, -1, 0}};
struct sembuf free_resource[1] = {{RES_FREE, 1, 0}};
struct sembuf wakeup_server[1] = {{0, 1, 0}};
struct sembuf wait_server[1] = {{1, -1, 0}};
int main()
{
    int a, b;
    long result;
    pid_t my_pid;
    my_pid = getpid();
    printf("My pid = %d\n", (int)my_pid);
    key = ftok("/bin/ls", '1');
    shmid = shmget(key, sizeof(mem_t), 0);
    semid = semget(key, NUMSEMS, 0);
    if ((shmid == -1) || (semid == -1)) {
        printf("ipc get failed\n");
        return 1;
    }

    data = (mem_t*)shmat(shmid, NULL, 0);
    if (data == (mem_t*)(-1)) {
        printf("shmat failed\n");
        return 2;
    }


    while (!feof(stdin)) {
        printf("Input >:\n");
        scanf("%d %d", &a, &b);
   
        semop(semid, use_resource, 1);

        data->a = a;
        data->b = b;
        data->client = my_pid;
    
        semop(semid, wakeup_server, 1);
        semop(semid, wait_server, 1);

        result = data->result;

        semop(semid, free_resource, 1);
        printf("Result >: %ld\n", result);
    }
    shmdt(data);
    return 0;
}
