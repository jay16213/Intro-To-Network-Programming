#include "headers.h"

int main(int argc, char **argv)
{
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct ifconf ifc;
    struct ifreq *ifr;

    int len = 100 * sizeof(struct ifreq);
    char *buf = (char *) malloc(len);

    ifc.ifc_len = len;
    ifc.ifc_buf = buf;
    ifr = (struct ifreq *) buf;

    if(ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
    {
        perror("ioctl error");
        exit(-1);
    }

    char *ptr = buf;

    ifr = (struct ifreq *) ptr;

    if(argc == 3)
    {
        struct sockaddr_in new_addr;
        new_addr.sin_family = AF_INET;
        inet_pton(AF_INET, argv[2], &(new_addr.sin_addr));
        memcpy(&(ifr->ifr_addr), &new_addr, sizeof(struct sockaddr));

        if(ioctl(sockfd, SIOCSIFADDR, ifr) < 0)
        {
            perror("set ip error");
        }
        return 0;
    }
    printf("lo -> inet addr: %s\n", inet_ntoa(((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr));

    free(buf);
    return 0;
}