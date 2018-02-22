#include "unp.h"

void server(int listenfd)
{
    struct Client client[FD_SIZE];
    socklen_t client_len;
    int connfd, maxfd, maxi = -1;
    int num_of_ready, i;
    fd_set read_set, all_set;
    struct sockaddr_in client_addr;
    char sendline[BUF_SIZE], recvline[BUF_SIZE];

    //init
    maxfd = listenfd;
    for(i = 0; i < FD_SIZE; i++) client[i].occupied = 0;
    FD_ZERO(&all_set);
    FD_SET(listenfd, &all_set);

    while(1)
    {
        read_set = all_set;
        num_of_ready = Select(maxfd+1, &read_set, NULL, NULL, NULL);

        //new client
        if(FD_ISSET(listenfd, &read_set))
        {
            client_len = sizeof(client_addr);
            connfd = Accept(listenfd, (struct sockaddr *) &client_addr, &client_len);
            char client_ip[INET_ADDRSTRLEN];
            Inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            printf("New client from: %s:%d\n", client_ip, ntohs(client_addr.sin_port));

            for(i = 0; i < FD_SIZE; i++)
            {
                if(!client[i].occupied)
                {
                    setClient(&client[i], connfd, client_addr, client_ip, ntohs(client_addr.sin_port));
                    snprintf(sendline, BUF_SIZE, "[Server] Hello, anonymous! From: %s:%d\n", client_ip, ntohs(client_addr.sin_port));
                    Writen(connfd, sendline, strlen(sendline));
                    snprintf(sendline, BUF_SIZE, "[Server] Someone is coming!\n");
                    Broadcast(client, i, sendline);
                    client[i].occupied = 1;
                    break;
                }
            }

            if(i == FD_SIZE)
                fprintf(stdout, "Error: too many clients, please try again.\n");

            FD_SET(connfd, &all_set);
            if(connfd > maxfd) maxfd = connfd;
            if(i > maxi) maxi = i;

            if(--num_of_ready <= 0) continue;//no more readable descriptors
        }

        memset(sendline, 0, BUF_SIZE * sizeof(char));
        memset(recvline, 0, BUF_SIZE * sizeof(char));
        ssize_t n;
        for(i = 0; i <= maxi; i++)
        {
            if(!client[i].occupied) continue;

            if(FD_ISSET(client[i].fd, &read_set))
            {
                if(Read(client[i].fd, recvline, BUF_SIZE))
                {
                    fprintf(stdout, "[%s:%d] %s", client[i].ip, client[i].port, recvline);

                    char broadcast_msg[BUF_SIZE], private_msg[BUF_SIZE];
                    int recevier = -1, bcast = i;
                    memset(broadcast_msg, 0, BUF_SIZE * sizeof(char));
                    memset(private_msg, 0, BUF_SIZE * sizeof(char));

                    int c = commandHandler(&bcast, recvline, client, &client[i], &recevier, sendline, private_msg, broadcast_msg);
                    //for telnet connection closing
                    if(c == 1)
                    {
                        close(client[i].fd);
                        FD_CLR(client[i].fd, &all_set);
                        client[i].occupied = 0;
                        fprintf(stdout, "client %s:%d exit.\n", client[i].ip, client[i].port);
                        snprintf(sendline, BUF_SIZE, "[Server] %s is offline.\n", client[i].name);
                        Broadcast(client, i, sendline);
                        continue;
                    }

                    Writen(client[i].fd, sendline, strlen(sendline));

                    memset(sendline, 0, BUF_SIZE * sizeof(char));
                    memset(recvline, 0, BUF_SIZE * sizeof(char));

                    if(recevier >= 0)
                        Writen(recevier, private_msg, strlen(private_msg));

                    if(strlen(broadcast_msg))
                        Broadcast(client, bcast, broadcast_msg);
                }
                else
                {
                    close(client[i].fd);
                    FD_CLR(client[i].fd, &all_set);
                    client[i].occupied = 0;
                    fprintf(stdout, "client %s:%d exit.\n", client[i].ip, client[i].port);
                    snprintf(sendline, BUF_SIZE, "[Server] %s is offline.\n", client[i].name);
                    Broadcast(client, i, sendline);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("Usage: ./a.out <Port>\n");
        exit(EXIT_FAILURE);
    }

    int listenfd;
    struct sockaddr_in server_addr;
    char ip[INET_ADDRSTRLEN];

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    Bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    Listen(listenfd, LISTENQ);

    Inet_ntop(AF_INET, &(server_addr.sin_addr), ip, INET_ADDRSTRLEN);
    printf("Server is now listening on %s at ip %s\n", argv[1], ip);

    server(listenfd);
    return 0;
}