#ifndef HEADERS_H
#define HEADERS_H

#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <strings.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#define RECV_TIMEO_USEC 100000
#define SEND_TIMEO_USEC 250000

#define FILENAME 'f'
#define FILESIZE 's'
#define DATA 'd'
#define ACK 'a'

#define DATA_BEGIN 2

#define INT_SIZE 4
#define SEG_SIZE 1024
#define PKT_SIZE 1033

char *u32intToBits(uint32_t num);
uint32_t bitsTou32int(char *bits);

struct msghdr* getEmptyBuf(struct sockaddr_in *sock_addr, socklen_t addrlen);
void setMsghdr(struct msghdr *msg, char type, uint32_t sn, char *data, uint32_t data_bytes);
void freeMsghdr(struct msghdr *msg);
int readable_timeo(int fd, int usec);
#endif
