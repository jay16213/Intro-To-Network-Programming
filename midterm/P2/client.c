#include "client.h"

int main(int argc, char **argv)
{
    if(argc < 3)
    {
        printf("Usage: ./client <IP addr> <Port>.\n");
        exit(-1);
    }

    int sockfd;
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));

    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    if(connect(sockfd, (SA *) &server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect error");
        exit(-1);
    }

    while(1)
    {
        char cmd[MAXLINE], res[MAXLINE];

        fgets(cmd, MAXLINE, stdin);
        write(sockfd, cmd, strlen(cmd));

        int n = read(sockfd, res, MAXLINE);
        if(n == 0)
        {
            fputs("The server has closed the connection.\n", stdout);
            break;
        }

        res[n] = '\0';
        fputs(res, stdout);
        memset(cmd, 0, MAXLINE);
        memset(res, 0, MAXLINE);
    }
    return 0;
}