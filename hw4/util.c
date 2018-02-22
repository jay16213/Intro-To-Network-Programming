#include "headers.h"

void newClient(struct Client *cli, int cmdLink, int dataLink, char *username)
{
    cli->cmdLink = cmdLink;
    cli->dataLink = dataLink;
    strcpy(cli->name, username);
    cli->cmdsbuf = (char *) malloc(CMDLINE * sizeof(char));
    cli->cmdrbuf = (char *) malloc(CMDLINE * sizeof(char));
    cli->sendbuf = (char *) malloc(DATALINE * sizeof(char));
    cli->recvbuf = (char *) malloc(DATALINE * sizeof(char));

    cli->cmdsiptr = cli->cmdsoptr = cli->cmdsbuf;
    cli->cmdriptr = cli->cmdroptr = cli->cmdrbuf;
    cli->sendiptr = cli->sendoptr = cli->sendbuf;
    cli->recviptr = cli->recvoptr = cli->recvbuf;
    return;
}

void freeClient(struct Client *cli)
{
    cli->occupied = 0;
    free(cli->cmdsbuf);
    free(cli->cmdrbuf);
    free(cli->sendbuf);
    free(cli->recvbuf);
    cli->cmdQueue = 0;
    cli->fileSize = 0;
    fprintf(stdout, "Client %s leave.\n", cli->name);
    return;
}

ssize_t nonBlockRead(int fd, void *buf, size_t count)
{
    ssize_t n;
    if((n = read(fd, buf, count)) == -1)
    {
        if(errno != EWOULDBLOCK)
        {
            perror("read error");
            exit(-1);
        }
    }

    return n;
}

ssize_t nonBlockWrite(int fd, const void *vptr, size_t count)
{
    ssize_t n;
    if((n = write(fd, vptr, count)) < 0)
    {
        if(errno != EWOULDBLOCK)
        {
            perror("write error");
            exit(-1);
        }
    }

    return n;
}

ssize_t Writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while(nleft > 0)
    {
        if((nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if(nwritten < 0 && errno == EINTR)
                nwritten = 0;
            else
            {
                perror("Written error");
                return -1;
            }
        }

        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

FILE *Fopen(const char *pathname, const char *mode)
{
    FILE *fp = fopen(pathname, mode);
    if(fp == NULL)
    {
        perror("fopen error");
        exit(-1);
    }

    return fp;
}

ssize_t Read(int fd, void *buf, size_t count)
{
    ssize_t n;
    if((n = read(fd, buf, count)) == -1)
    {
        perror("Read error");
        return -1;
    }
    return n;
}

int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    int n;
    if((n = select(nfds, readfds, writefds, exceptfds, timeout)) == -1)
    {
        perror("select error");
        exit(-1);
    }
    return n;
}

uint32_t getFileSize(FILE *fp)
{
    fseek(fp, 0, SEEK_END);//jump to the end of file
    uint32_t fileSize = ftell(fp);  //get current byte offset in the file
    rewind(fp);            //jump back to the beginning of the file
    return fileSize;
}

int printProgress(uint32_t sent, uint32_t total)
{
    int num = ((double) sent / total) * 20;
    if(num >= 20) num = 20;
    int left = 20 - num;
    int i;

    fprintf(stdout, "Progress : [");
    for(i = 0; i < num; i++) fprintf(stdout, "#");
    for(i = 0; i < left; i++) fprintf(stdout, " ");
    fprintf(stdout, "]");

    fflush(stdout);
    if(num == 20)
        printf("\n");
    else
        printf("\r");
    return num == 20;
}

void closeFile(struct Client *cli)
{
    fclose(cli->fp);
    cli->fp = NULL;
    cli->flags = C_IDLE;
    memset(cli->filename, 0, NAME_SIZE);
    return;
}
