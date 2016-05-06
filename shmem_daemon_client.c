#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>

#include "misc.h"

key_t key;
int shmid;
int semid;

mem_t* data;
struct sembuf free_resource[1] = {{RES_FREE, 1, 0}};
struct sembuf wakeup_server[1] = {{0, 1, 0}};
struct sembuf wait_server[1] = {{1, -1, 0}};
int main(int argc, char** argv)
{
    size_t writed;
	  size_t file_name_length;

    pid_t my_pid;
    my_pid = getpid();
    printf("My pid = %d\n", (int)my_pid);
    key = ftok("/bin/ls", '1');
    shmid = shmget(key, sizeof(mem_t), 0);
    semid = semget(key, NUMSEMS, 0);
    if ((shmid == -1) || (semid == -1))
    {
        perror("ipc get failed");
        return 1;
    }

    data = (mem_t*)shmat(shmid, NULL, 0);
    if (data == (mem_t*)(-1))
    {
        perror("shmat failed\n");
        return 2;
    }

    if(argc < 2)
    {
        fprintf(stdout, "invalid arguments\n");
        return 1;
    }

    printf("Input file name>: %s\n", argv[1]);

    writed = 0;

    file_name_length = strlen(argv[1]);
    data->pk_type = PK_SEND_FILENAME;
    while(writed <= file_name_length)
    {
        memset(data->data, 0, BUF_SIZE);
        strncpy(data->data, argv[1] + writed, BUF_SIZE);
        semop(semid, wakeup_server, 1);
        semop(semid, wait_server, 1);
        if(data->pk_type == PK_ERROR)
        {
            fprintf(stderr, "server error %s", data->data);
            return 1;
        }
        if(data->pk_type != PK_OK)
        {
            fprintf(stderr, "something wrong\n");
            return 1;
        }
        writed += BUF_SIZE;
    }
    data->pk_type = PK_FILENAME_OK;
    semop(semid, wakeup_server, 1);
    semop(semid, wait_server, 1);
    if(data->pk_type == PK_ERROR)
    {
        fprintf(stderr, "server error %s\n", data->data);
        return 1;
    }
    while(data->pk_type == PK_SEND_DATA)
    {
        fprintf(stdout, "%s", data->data);
        data->pk_type = PK_OK;
        semop(semid, wakeup_server, 1);
        semop(semid, wait_server, 1);
        if(data->pk_type == PK_ERROR)
        {
            fprintf(stderr, "server error\n");
            return 1;
        }
    }
    if(data->pk_type != PK_EOF)
    {
        fprintf(stderr, "server error\n");
        return 1;
    }
    fprintf(stdout, "%s", data->data);

    semop(semid, free_resource, 1);
    shmdt(data);
    return 0;
}
