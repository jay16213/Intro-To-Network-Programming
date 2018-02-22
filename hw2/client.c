#include "unp.h"

int main(int argc, char **argv)
{
    if(argc < 3)
    {
        printf("Usage: ./a.out <IPaddress> <Port>.\n");
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints, *res;
    struct in_addr host_addr;
    char host_ip[16];

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    Getaddrinfo(argv[1], NULL, &hints, &res);
    host_addr.s_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    Inet_ntop(AF_INET, &host_addr, host_ip, sizeof(host_ip));
    printf("Connect to %s (%s)...\n", argv[1], host_ip);
    freeaddrinfo(res);

    int sockfd;
    struct sockaddr_in server_addr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket error.\n");
        printf("Can not create the client socket, please try again.\n");
        exit(EXIT_FAILURE);
    }

    //clear the data
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));

    if(inet_pton(AF_INET, host_ip, &server_addr.sin_addr) <= 0)
        printf("inet_pton error for %s\n", argv[1]);

    if(connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connect error");
        switch(errno)
        {
            case ECONNREFUSED:
                printf("connect refused.\n");
            break;
            default:
                printf("other errors\n");
            break;
        }
    }

    int maxfd;
    fd_set read_set;

    FD_ZERO(&read_set);
    int not_exit = 1;
    while(not_exit)
    {
        char sendline[BUF_SIZE], recvline[BUF_SIZE];
        FD_SET(fileno(stdin), &read_set);
        FD_SET(sockfd, &read_set);

        maxfd = sockfd + 1;

        Select(maxfd, &read_set, NULL, NULL, NULL);

        //read from sockfd
        if(FD_ISSET(sockfd, &read_set))
        {
            Read(sockfd, recvline, BUF_SIZE);
            fputs(recvline, stdout);
            memset(recvline, 0, BUF_SIZE * sizeof(char));
        }

        //read from input
        if(FD_ISSET(fileno(stdin), &read_set))
        {
            if(fgets(sendline, BUF_SIZE, stdin) == NULL) return 0;

            char tmp[BUF_SIZE];
            strcpy(tmp, sendline);
            char *cmd = strtok(tmp, TOK_DELIMETER);

            if(cmd != NULL && strcmp(cmd, "exit") == 0)
            {
                not_exit = 0;
                break;
            }

            Writen(sockfd, sendline, strlen(sendline));
            memset(sendline, 0, BUF_SIZE * sizeof(char));
        }
    }

    return 0;
}