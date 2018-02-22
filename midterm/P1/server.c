#include "server.h"

#define LISTENQ 32

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        exit(0);
    }

    int listenfd;
    struct sockaddr_in server_addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    bind(listenfd, (SA *) &server_addr, sizeof(server_addr));
    if(listen(listenfd, LISTENQ) == -1)
    {
        perror("listen error");
        exit(-1);
    }

    int connfd;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    while(1)
    {
        client_len = sizeof(client_addr);
        if((connfd = accept(listenfd, (SA *) &client_addr, &client_len)) == -1)
        {
            perror("Accept error");
            exit(-1);
        }

        while(1)
        {
            char cmd[MAXLINE];
            int n = read(connfd, cmd, MAXLINE);
            if(n <= 0)
            {
                close(connfd);
                break;
            }

            cmd[n] = '\0';
            fputs(cmd, stdout);
            calc(connfd, cmd);
        }
    }
    return 0;
}