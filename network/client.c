#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <netinet/ip.h>
#include <netinet/in.h>

#define MAGIC_CONST 5
#define THREADS 3

int port_num;
char* host_name;
int sock_id;

struct hostent *hst;
struct sockaddr_in server_addr;
FILE* file;

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        printf("fail argc\n");
        return -1;
    }
    port_num = atoi(argv[1]);
    host_name = argv[2];
    
    sock_id = socket(PF_INET, SOCK_STREAM, 0);
    hst = gethostbyname(host_name);

   
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((short)port_num);
    memcpy(&(server_addr.sin_addr.s_addr),
            hst->h_addr_list[0], hst->h_length);
    
    if (connect(sock_id, (struct sockaddr*)(&server_addr),
            sizeof(struct sockaddr_in)) == -1)
    {
        printf("connect fail\n");
        return -2;
    }

    shutdown(sock_id, SHUT_WR);
    file = fdopen(sock_id, "r");

    while (!feof(file))
    {
        printf("%c", fgetc(file));
    }
    
    printf("\nEND\n");
    fclose(file);
    return 0;
}
