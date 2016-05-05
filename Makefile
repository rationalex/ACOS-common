CC = clang
CFLAGS = -Wextra -Wall -Werror -pedantic

all: server client shmem_daemon shmem_daemon_client

server: shared_memory_server.c
	$(CC) shared_memory_server.c $(CFLAGS) -o server

client: shared_memory_client.c
	$(CC) shared_memory_client.c $(CFLAGS) -o client

shmem_daemon: shmem_daemon.c
	$(CC) shmem_daemon.c $(CFLAGS) -o shmem_daemon

shmem_daemon_client: shmem_daemon_client.c
		$(CC) shmem_daemon_client.c $(CFLAGS) -o shmem_daemon_client

clean:
	rm *.o* server client shmem_daemon shmem_daemon_client -rf
