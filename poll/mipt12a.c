#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <netinet/ip.h>
#include <netinet/in.h>

#define MAGIC_CONST 5
#define CLIENTS 3

int port_num;
int bind_ret;
int acc_ret;
struct sockaddr_in server_addr;

volatile int numconnected;
/** poll_arr[0] - sock_id,
 * poll_arr[CLIENTS + 1] - looser
 */
struct pollfd poll_arr[CLIENTS + 2];
int wrote[CLIENTS + 2];

int i, j;

void handler(int sig)
{
    if (sig == SIGINT)
    {
        int i;
        for (i = 0; i < CLIENTS + 2; ++i) {
            if (poll_arr[i].fd >= 0) {
                close(poll_arr[i].fd);
            }    
        }
        exit(0);
    }
}

const char msgok[] = "hello";
const char msgerr[] = "error";

int main(int argc, char** argv)
{
    numconnected = 0;
    
    if (argc != 2)
    {
        printf("fail argc\n");
        return -1;
    }
    port_num = atoi(argv[1]);
    poll_arr[0].fd = socket(PF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((short)port_num);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bind_ret = bind(poll_arr[0].fd, (struct sockaddr*)(&server_addr),
            sizeof(struct sockaddr_in));
    if (bind_ret == -1)
    {
        printf("fail bind\n");
        return -2;
    }
    listen(poll_arr[0].fd, MAGIC_CONST);
    signal(SIGINT, handler);

    poll_arr[0].events = POLLIN | POLLERR | POLLHUP;
    poll_arr[0].revents = 0;
    wrote[0] = 0;
    for (i = 1; i < CLIENTS + 2; ++i)
    {
        poll_arr[i].fd = -1; /* ignore */
        poll_arr[i].revents = 0;
        wrote[i] = 0; 
    }
    while (1)
    {
        int poll_ret;
        poll_ret = poll(poll_arr, CLIENTS + 2, -1);
        if (poll_ret == -1)
        {
            perror("poll failed\n");
            handler(SIGINT);
        }
        
        /* poll_arr[0] processing */
        if (poll_arr[0].revents & POLLERR) {
            printf("poll returned POLLERR\n");
            handler(SIGINT);
        }
        if (poll_arr[0].revents & POLLHUP) {
            /** never happens */
            printf("poll returned POLLHUP\n");
            handler(SIGINT);
        }
        if (poll_arr[0].revents & POLLIN) {
            int connected = 0;
            for (j = 1; j <= CLIENTS; ++j) {
                if (poll_arr[j].fd >= 0) {
                    ++connected;
                }
            }
            if (!((connected == CLIENTS)
                    && (poll_arr[CLIENTS + 1].fd >= 0))) {

                acc_ret = accept(poll_arr[0].fd, NULL, NULL);
                if (acc_ret == -1) {
                    perror("accept failed");
                    handler(SIGINT);
                }

                if (connected < CLIENTS) {
                    for (j = 1; j <= CLIENTS; ++j) {
                        if (poll_arr[j].fd < 0) {
                            poll_arr[j].fd = acc_ret; 
                            poll_arr[j].events = POLLOUT | POLLERR | POLLHUP;
                            break;
                        }
                    }
                } else {
                    poll_arr[CLIENTS + 1].fd = acc_ret;
                    poll_arr[CLIENTS + 1].events = POLLOUT | POLLERR | POLLHUP;
                }
            }
        }
        /* poll_arr[0] processing end */
        for (i = 1; i <= CLIENTS; ++i)
        {
            if ((1 <= i) && (i <= CLIENTS)) {
                if (poll_arr[i].revents & POLLERR) {
                    printf("client poll failed : POLLERR\n");
                    handler(SIGINT);
                } 
                if (poll_arr[i].revents & POLLHUP) {
                    printf("client poll failed : POLLHUP\n");
                    handler(SIGINT);
                }
                if (poll_arr[i].revents & POLLOUT) {
                    wrote[i] += write(poll_arr[i].fd, msgok + wrote[i]
                            , strlen(msgok) + 1 - wrote[i]);
                    if (wrote[i] == strlen(msgok) + 1) {
                        close(poll_arr[i].fd);
                        wrote[i] = 0;
                        poll_arr[i].fd = -1;
                        poll_arr[i].revents = 0;
                    }
                }
            }
        }
        /* poll_arr[CLIENTS + 1] processing */ 
        if (poll_arr[CLIENTS + 1].revents & POLLERR) {
            printf("looser client poll failed : POLLERR\n");
            handler(SIGINT);
        } 
        if (poll_arr[CLIENTS + 1].revents & POLLHUP) {
            printf("looser client poll failed : POLLHUP\n");
            handler(SIGINT);
        }
        if (poll_arr[CLIENTS + 1].revents & POLLOUT) {
            wrote[CLIENTS + 1] += write(poll_arr[CLIENTS + 1].fd,
                    msgerr + wrote[CLIENTS + 1],
                    strlen(msgerr) + 1 - wrote[CLIENTS + 1]);
            if (wrote[CLIENTS + 1] == strlen(msgerr) + 1) {
                close(poll_arr[CLIENTS + 1].fd);
                wrote[CLIENTS + 1] = 0;
                poll_arr[CLIENTS + 1].fd = -1;
                poll_arr[CLIENTS + 1].revents = 0;
            }
        }
        /* poll_arr[CLIENTS + 1] processing end */
    }

    return 0;
}
