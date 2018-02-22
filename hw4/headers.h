#ifndef _HEADER_H_
#define _HEADER_H_

#include <math.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
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

#define DELI " \a\t\r\n"

#define LISTENQ         128
#define CLI_SIZE         64
#define TMP_SIZE        128
#define NAME_SIZE       256

#define CMD_SIZE        265
#define CMDLINE        7950//30 cmd

#define DATALINE     103300//100 pkts
#define SEG_SIZE       1024
#define PKT_SIZE       1033

#define FILELIST_SIZE  8192
#define DATA              0
#define F_DATA            1
#define ACK               2
#define WELCOME           4

#define C_CONNECTING      1
#define C_UPLOADING       2
#define C_DOWNLOADING     4
#define C_IDLE            8

struct File {
    char *username;
    char *filename;
    uint32_t fileSize;
} fileList[FILELIST_SIZE];

struct Client {
    int occupied;
    int dataLink;
    int cmdLink;
    int flags;
    char name[NAME_SIZE];
    FILE *fp;
    char filename[NAME_SIZE];
    uint32_t fileSize;
    int sn;
    char final;
    char *cmdsbuf;
    char *cmdsiptr;
    char *cmdsoptr;
    char *cmdrbuf;
    char *cmdriptr;
    char *cmdroptr;
    int cmdQueue;
    char *sendbuf;
    char *sendiptr;
    char *sendoptr;
    char *recvbuf;
    char *recviptr;
    char *recvoptr;
} client[CLI_SIZE];

struct {
    int fd;
    int sent;
} tmpfd[TMP_SIZE];
int numOfFiles;

ssize_t nonBlockRead(int fd, void *buf, size_t count);
ssize_t nonBlockWrite(int fd, const void *vptr, size_t count);
void newClient(struct Client *cli, int cmdLink, int dataLink, char *username);
void freeClient(struct Client *cli);
ssize_t Writen(int fd, const void *vptr, size_t n);
ssize_t Read(int fd, void *buf, size_t count);
FILE *Fopen(const char *pathname, const char *mode);
int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
void syncFileRequest(struct Client *cli);
void addFile(char *username, char *filename, uint32_t fileSize);
uint32_t getFileSize(FILE *fp);
int printProgress(uint32_t sent, uint32_t total);
void closeFile(struct Client *cli);
#endif
