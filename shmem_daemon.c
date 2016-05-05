#include <pwd.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/file.h>
#include <sys/types.h>
#include <stdio.h>

#include "misc.h"

key_t key;
int shmid;
int semid;

mem_t* data;
struct sembuf wait_client[1] = {{0, -1, 0}};
struct sembuf send_client[1] = {{1, 1, 0}};

pid_t pid;
int pid_file_fd;

void usr1_handler(int sign)
{
    if (sign == SIGUSR1)
    {
        syslog(LOG_NOTICE, "SIGUSR1 received");
    }
    signal(SIGUSR1, usr1_handler);
}

int main()
{
    uid_t uid;
    size_t bytes_read;

    FILE *pid_file;
    FILE *client_file;

    char *file_name = NULL;
    char *tmp;

    uid = geteuid();
    if (uid != 0)
    {
        fprintf(stderr, "Sorry, root is required\n");
          return -1;
    }

    pid_file_fd = open("/var/run/" DAEMON_NAME,
                       O_CREAT | O_WRONLY,
                       0644);

    if (pid_file_fd == -1)
    {
        perror("open error");
        return -1;
    }

    if (flock(pid_file_fd, LOCK_EX | LOCK_NB) == -1)
    {
        perror("block failed");
        return -1;
    }

    openlog(DAEMON_NAME, LOG_CONS, LOG_DAEMON);

    struct passwd *pass = getpwnam(DAEMON_USERNAME);
    if (pass == NULL)
    {
        fprintf(stderr, "No such user: %s\n", DAEMON_USERNAME);
        return -1;
    }

    uid = pass->pw_uid;
    seteuid(uid);
    if (setpgrp() == -1)
    {
        perror("changing gid error");
        return -1;
    }

    pid = fork();
    if (pid != 0) /* parent process: we should exit */
    {
        return 0;
    }

    /* now we are in child(daemon) process */
    if (setsid() == -1) /* creating new session so daemon isn't killed by stopping initial terminal process */
    {
        perror("creating session error");
        return -1;
    }

    pid = getpid();

    pid_file = fdopen(pid_file_fd, "w");
    if (pid_file == NULL)
    {
        syslog(LOG_ERR, "Could manage to open fd [%d]\n", pid_file_fd);
        return -1;
    }

    fprintf(pid_file, "%ld", (long)pid);
    fflush(pid_file);

    /* closing standart fd */
    close(0);
    close(1);
    close(2);

    /* now main code part starts */
    key = ftok(SHMEMPATH, SHMEMKEY);
    shmid = shmget(key, sizeof(mem_t), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("shared memory id get error");
        return 1;
    }

    semid = semget(key, NUMSEMS, IPC_CREAT | 0666);
    if (semid == -1)
    {
        printf("semaphors id get failed\n");
        return 1;
    }

    data = (mem_t*)shmat(shmid, NULL, 0);
    if (data == NULL)
    {
        printf("shmat failed\n");
        return 2;
    }

    while (1)
    {
        int file_name_len = 0;
        semop(semid, wait_client, 1);
        switch (data->pk_type)
        {
        /* read packets until end of filename */
        case PK_SEND_FILENAME:
            file_name_len += BUF_SIZE;
            tmp = realloc(file_name, file_name_len * sizeof(char));
            if (tmp == NULL)
            {
                syslog(LOG_ERR, "filename realloc error");
                if (file_name != NULL) free(file_name);
                snprintf(data->data, BUF_SIZE, "Sorry, filename is too long for me\n");
                data->pk_type = PK_ERROR;
            }
            else
            {
                file_name = tmp;
                strncpy(file_name + (file_name_len - BUF_SIZE),
                        data->data,
                        BUF_SIZE);
                data->pk_type = PK_OK;
            }
            break;

        /* client finished sending packets with filename
         * previous packet was last one
         */
        case PK_FILENAME_OK:
            client_file = fopen(file_name, "r");

            if (client_file == NULL)
            {
                data->pk_type = PK_ERROR;
				memset(data->data, 0, BUF_SIZE);
                snprintf(data->data,
                         BUF_SIZE,
                         "Can't open file\n");
            }
        /* now we need to send some data to client */
        case PK_OK:
			memset(data->data, 0, BUF_SIZE);
            if (feof(client_file))
            {
                data->pk_type = PK_EOF;
                data->read_len = 0;
            }
            else
            {
                bytes_read = fread(data->data, sizeof(char), BUF_SIZE, client_file);
                data->read_len = bytes_read;
				if(feof(client_file))
					data->pk_type = PK_EOF;
				else
					data->pk_type = PK_SEND_DATA;
            }
            break;
        case PK_ERROR:
        case PK_END:
            if (client_file != NULL)
            {
                fclose(client_file);
            }
            if (file_name != NULL)
            {
                free(file_name);
            }
            break;
        /* if packet type isn't one of documentised */
        default:
            syslog(LOG_ERR, "Wrong packet content");
            if (client_file != NULL)
            {
                fclose(client_file);
            }
            if (file_name != NULL)
            {
                free(file_name);
            }
            break;
        }
        semop(semid, send_client, 1);
    }

    return 0;
}
