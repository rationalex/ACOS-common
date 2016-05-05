CC = clang
CFLAGS = -ansi -Wall -Wextra -g -D _GNU_SOURCE

all: shmem_daemon shmem_daemon_client  #shmem_server shmem_client

#shmem_server: shared_memory_server.c
#	$(CC) shared_memory_server.c $(CFLAGS) -o shmem_server

#shmem_client: shared_memory_client.c
#		$(CC) shared_memory_client.c $(CFLAGS) -o shmem_client

shmem_daemon: shmem_daemon.c misc.h
	$(CC)  shmem_daemon.c $(CFLAGS) -o shmem_daemon

shmem_daemon_client: shmem_daemon_client.c misc.h
	$(CC)  shmem_daemon_client.c $(CFLAGS) -o shmem_daemon_client

clean:
	rm -f shmem_daemon shmem_daemon_client shmem_server shmem_client
