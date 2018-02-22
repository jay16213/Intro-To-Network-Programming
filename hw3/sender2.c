#include "headers.h"
#include "sender2.h"

static void sig_alarm(int signo)
{
    return;
}

int main(int argc ,char **argv)
{
    if(argc < 4)
    {
        printf("Usage: ./sender2 <receiver IP> <receiver port> <file name>\n");
        exit(0);
    }

    if(strlen(argv[3]) >= 256)
    {
        printf("The filename must be less than 256 characters\n");
        exit(0);
    }

    int recvfd;
    struct addrinfo hints, *res;
    struct sockaddr_in receiver_addr;
    char receiver_ip[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;

    if(getaddrinfo(argv[1], NULL, &hints, &res) != 0)
    {
        perror("getaddrinfo error");
        exit(EXIT_FAILURE);
    }

    struct in_addr host_addr;
    host_addr.s_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    inet_ntop(AF_INET, &host_addr, receiver_ip, INET6_ADDRSTRLEN);
    printf("Connect to %s (%s)...\n", argv[1], receiver_ip);

    recvfd = socket(AF_INET, SOCK_DGRAM, 0);
    signal(SIGALRM, sig_alarm);
    siginterrupt(SIGALRM, 1);

    struct sockaddr_in recv_addr;
    bzero(&recv_addr, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(atoi(argv[2]));

    if(inet_pton(AF_INET, receiver_ip, &recv_addr.sin_addr) <= 0)
        printf("inet_pton error for %s\n", argv[1]);

    //transfer file
    sendFile(recvfd, &recv_addr, res->ai_addrlen, argv[3]);
    freeaddrinfo(res);
    return 0;
}
