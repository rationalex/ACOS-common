all: server client

server: shmem_daemon.c misc.h
	gcc -ansi -Wall -Wextra -D _GNU_SOURCE shmem_daemon.c -o server -g

client: shared_memory_client.c misc.h
	gcc -ansi -Wall -Wextra -D _GNU_SOURCE shared_memory_client.c -o client -g

clean:
	rm -f server client
