#include "headers.h"

#define BUF_SIZE 1500

char sendbuf[BUF_SIZE];
int datalen = 56;
char *host;
int nsent;
pid_t pid;
int sockfd;
char tempWa[200];

void tv_sub(struct timeval *out, struct timeval *in)
{
    if((out->tv_usec -= in->tv_usec) < 0)
    {
        --out->tv_sec;
        out->tv_usec += 1000000;
    }

    out->tv_sec -= in->tv_sec;
}

struct proto {
    struct hostent *host;
    struct sockaddr *sasend;
    struct sockaddr *sarecv;
    socklen_t salen;
    int icmpproto;
} pr;

uint16_t in_cksum(uint16_t *addr, int len);
struct addrinfo *host_serv(const char *host, const char *serv, int family, int sockType);
char *sock_n_top_host(const struct sockaddr *sa, socklen_t salen);
int proc_icmp(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv, double *rtt);
void send_icmp();

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("Usage: sudo ./pingsubnet_seq <ip>.\n");
        exit(-1);
    }

    struct addrinfo *addrIn;
    char *h;

    pid = getpid() & 0xffff;

    int subnet;
    for(subnet = 1; subnet <= 254; subnet++)
    {
        char target_ip[16];
        snprintf(target_ip, 16, "%s.%d", argv[1], subnet);
        //printf("%s\n", target_ip);

        addrIn = host_serv(target_ip, NULL, 0, 0);
        h = sock_n_top_host(addrIn->ai_addr, addrIn->ai_addrlen);

        pr.sasend = addrIn->ai_addr;
        pr.sarecv = calloc(1, addrIn->ai_addrlen);
        pr.salen = addrIn->ai_addrlen;
        pr.icmpproto = IPPROTO_ICMP;
        in_addr_t ipAddr = inet_addr(target_ip);

        struct sockaddr_in tempSa;
        tempSa.sin_family = AF_INET;
        inet_pton(AF_INET, target_ip, &tempSa.sin_addr);
        getnameinfo((const struct sockaddr *) &tempSa, sizeof(tempSa), tempWa, sizeof(tempWa), NULL, 0, 0);

        char recvbuf[BUF_SIZE];
        char ctrlbuf[BUF_SIZE];
        struct msghdr msg;
        struct iovec iov;
        ssize_t n;
        struct timeval tval, sendTval;

        if((sockfd = socket(pr.sasend->sa_family, SOCK_RAW, pr.icmpproto)) < 0)
        {
            perror("socket error");
            exit(-1);
        }

        int size = 60*1024;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

        struct timeval timeoutTv;
        timeoutTv.tv_sec = 3;
        timeoutTv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeoutTv, sizeof(timeoutTv));

        int count;
        double rtt[3];
        rtt[0] = rtt[1] = rtt[2] = -1.0;

        printf("%s (%s) ", target_ip, tempWa);
        fflush(stdout);

        for(count = 0; count < 3; count++)
        {
            send_icmp(&sendTval);

            iov.iov_base = recvbuf;
            iov.iov_len = sizeof(recvbuf);
            msg.msg_name = pr.sarecv;
            msg.msg_iov =  &iov;
            msg.msg_iovlen = 1;
            msg.msg_control = ctrlbuf;

            int result;
            struct timeval waitTv;

            do {
                msg.msg_namelen = pr.salen;
                msg.msg_controllen = sizeof(ctrlbuf);

                n = recvmsg(sockfd, &msg, 0);

                if(n < 0)
                {
                    if(errno == EINTR) continue;
                    if(errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        result = 0;
                        break;
                    }
                    else
                    {
                        perror("recvmsg error");
                        exit(-1);
                    }
                }

                gettimeofday(&tval, NULL);
                result = proc_icmp(recvbuf, n, &msg, &tval, &rtt[count]);

                waitTv = sendTval;
                tv_sub(&waitTv, &tval);
            } while(!result && waitTv.tv_sec < 3);

            if(count) printf(",");
            printf("RTT%d=", count + 1);
            if(rtt[count] > 0.0) printf("%.3fms", rtt[count]);
            else printf("*");
            fflush(stdout);
            
            if(result) sleep(1);
        }

        printf("\n");
    }

    return 0;
}

uint16_t in_cksum(uint16_t *addr, int len)
{
    int nleft = len;
    uint32_t sum = 0;
    uint16_t *w = addr;
    uint16_t answer = 0;

    while(nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if(nleft == 1)
    {
        *(unsigned char *) (&answer) = *(unsigned char *) w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

struct addrinfo *host_serv(const char *host, const char *serv, int family, int sockType)
{
    int n;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family- family;
    hints.ai_socktype = sockType;

    if((n = getaddrinfo(host, serv, &hints, &res)) != 0)
    {
        perror("geraddrinfo error");
        exit(-1);
    }

    return res;
}

char *sock_n_top_host(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];
    if(sa->sa_family == AF_INET)
    {
        struct sockaddr_in *sin = (struct sockaddr_in *) sa;
        if(inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)))
            return str;
    }
        
    return NULL;
}

int proc_icmp(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv, double *rtt)
{
    *rtt = -1.0;

    int hlen1, icmplen;
    struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend;
    char recvIp[128];

    inet_ntop(AF_INET, &((struct sockaddr_in *) pr.sarecv)->sin_addr, recvIp, sizeof(recvIp));

    ip = (struct ip *) ptr;
    hlen1 = ip->ip_hl << 2;
    if(ip->ip_p != IPPROTO_ICMP) return 0;
    icmp = (struct icmp *) (ptr + hlen1);
    if((icmplen = len - hlen1) < 8) return 0;

    if(icmp->icmp_type == ICMP_ECHOREPLY)
    {
        if(icmp->icmp_id != pid) return 0;
        if(icmplen < 16) return 0;

        tvsend = (struct timeval *) icmp->icmp_data;
        tv_sub(tvrecv, tvsend);
        *rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

        //printf("%s (%s) RTT=%.3fms\n", recvIp, tempWa, rtt);
        return 1;
    }
    else
        return 0;
}

void send_icmp()
{
    int len;
    struct icmp *icmp;

    icmp = (struct icmp *) sendbuf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = pid;
    icmp->icmp_seq = nsent++;
    memset(icmp->icmp_data, 0xa5, datalen);
    gettimeofday((struct timeval *) icmp->icmp_data, NULL);

    len = 8 + datalen;
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = in_cksum((u_short *) icmp, len);

    sendto(sockfd, sendbuf, len, 0, pr.sasend, pr.salen);
}
