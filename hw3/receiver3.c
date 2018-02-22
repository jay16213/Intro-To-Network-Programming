#include "headers.h"
#include "receiver3.h"

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("Usage: ./receiver1 <port>\n");
        exit(0);
    }

    int sockfd;
    struct sockaddr_in receiver_addr;

    //creatre a udp socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);


    bzero(&receiver_addr, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    receiver_addr.sin_port = htons(atoi(argv[1]));

    if(bind(sockfd, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)))
    {
        perror("Bind error");
        exit(0);
    }

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(receiver_addr.sin_addr), ip, INET_ADDRSTRLEN);
    printf("Receiver is binding at %s/%s\n", ip, argv[1]);

    while(1)
    {
        struct sockaddr_in sender_addr;
        receiver(sockfd, &sender_addr);
    }

    return 0;
}
