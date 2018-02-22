#include "server.h"

#define LISTENQ 32
#define MAXCLIENT 32
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

    int client[MAXCLIENT];
    int i;
    for(i = 0; i < MAXCLIENT; i++) client[i] = -1;

    fd_set r_set, all_set;
    int maxfd = listenfd;

    FD_ZERO(&all_set);
    FD_SET(listenfd, &all_set);

    while(1)
    {
        r_set = all_set;
        int num_of_ready;

        if((num_of_ready = select(maxfd + 1, &r_set, NULL, NULL, NULL)) == -1)
        {
            perror("select error");
            exit(-1);
        }

        if(FD_ISSET(listenfd, &r_set))
        {
            client_len = sizeof(client_addr);
            if((connfd = accept(listenfd, (SA *) &client_addr, &client_len)) == -1)
            {
                perror("Accept error");
                exit(-1);
            }

            for(i = 0; i < MAXCLIENT; i++)
            {
                if(client[i] < 0)
                {
                    fprintf(stdout, "New client.\n");
                    client[i] = connfd;
                    break;
                }
            }

            if(i == MAXCLIENT)
                fprintf(stdout, "too many client.\n");

            FD_SET(connfd, &all_set);
            if(connfd > maxfd) maxfd = connfd;
        }

        for(i = 0; i < MAXCLIENT; i++)
        {
            if(client[i] < 0) continue;
            int ter = 0;
            if(FD_ISSET(client[i], &r_set))
            {
                char cmd[MAXLINE];
                int n = read(client[i], cmd, MAXLINE);
                if(n <= 0)
                {
                    close(client[i]);
                    FD_CLR(client[i], &all_set);                
                    continue;
                }
    
                cmd[n] = '\0';
                fputs(cmd, stdout);
                ter = calc(client[i], cmd);
                if(ter) FD_CLR(client[i], &all_set);
            }
        }
    }
    return 0;
}