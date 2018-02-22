#include "headers.h"

int main(int argc, char **argv)
{
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket error");
        exit(-1);
    }

    struct sockaddr_in serv_addr;
    socklen_t servlen;
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(34567);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect error");
        exit(-1);
    }

    char sendbuf[1024];
    char recvbuf[1024];
    servlen = sizeof(serv_addr);

    sprintf(sendbuf, "req\n");
    if(sendto(sockfd, sendbuf, strlen(sendbuf) + 1, 0, (struct sockaddr *) &serv_addr, servlen) < 0)
    {
        perror("sendto error");
    }

    if(recvfrom(sockfd, recvbuf, 1024, 0, (struct sockaddr *) &serv_addr, &servlen) < 0)
    {
        perror("recvfrom error");
    }

    printf("client terminated.\n");
    return 0;
}