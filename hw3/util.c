#include "headers.h"

char *u32intToBits(uint32_t num)
{
    char *out = (char *) malloc(INT_SIZE * sizeof(char));
    memset(out, 0, INT_SIZE);

    int i, j, offset = 31;
    for(i = 0; i < 4; i++)
    {
        uint32_t c_offset = 7;
        for(j = 0; j < 8; j++)
        {
            uint32_t bit = (num & (1 << offset)) >> offset;
            out[i] |= bit << c_offset;
            c_offset--;
            offset--;
        }
    }

    return out;
}

uint32_t bitsTou32int(char *bits)
{
    uint32_t num = 0;

    int i, j, offset = 31;
    for(i = 0; i < 4; i++)
    {
        uint32_t c_offset = 7;
        for(j = 0; j < 8; j++)
        {
            uint32_t bit = (bits[i] & (1 << c_offset)) >> c_offset;
            num |= bit << offset;
            c_offset--;
            offset--;
        }
    }

    return num;
}

struct msghdr* getEmptyBuf(struct sockaddr_in *sock_addr, socklen_t addrlen)
{
    struct msghdr *msg = (struct msghdr *) malloc(sizeof(struct msghdr));
    memset(msg, 0, sizeof(struct msghdr));

    struct iovec *iov = (struct iovec *) malloc(4 * sizeof(struct iovec));

    char *type       = (char *) malloc(sizeof(char));
    char *str_sn     = (char *) malloc(INT_SIZE * sizeof(char));
    char *data       = (char *) malloc(SEG_SIZE * sizeof(char));
    char *data_bytes = (char *) malloc(INT_SIZE * sizeof(char));
    memset(type, 0, 2);
    memset(str_sn, 0, INT_SIZE);
    memset(data, 0, SEG_SIZE);
    memset(data_bytes, 0, INT_SIZE);

    iov[0].iov_base = type;
    iov[0].iov_len = 1;
    iov[1].iov_base = str_sn;
    iov[1].iov_len = INT_SIZE;
    iov[2].iov_base = data;
    iov[2].iov_len = SEG_SIZE;
    iov[3].iov_base = data_bytes;
    iov[3].iov_len = INT_SIZE;

    msg->msg_iov = iov;
    msg->msg_iovlen = 4;
    msg->msg_name = sock_addr;
    msg->msg_namelen = addrlen;
    return msg;
}

void setMsghdr(struct msghdr *msg, char type, uint32_t sn, char *data, uint32_t data_bytes)
{
    //clear buf
    memset(msg->msg_iov[0].iov_base, 0, 1);
    memset(msg->msg_iov[1].iov_base, 0, INT_SIZE);
    memset(msg->msg_iov[2].iov_base, 0, SEG_SIZE);
    memset(msg->msg_iov[3].iov_base, 0, INT_SIZE);

    memcpy(msg->msg_iov[0].iov_base, &type, 1);
    memcpy(msg->msg_iov[1].iov_base, u32intToBits(sn), INT_SIZE);

    if(type == DATA || type == FILESIZE)
    {
        memcpy(msg->msg_iov[2].iov_base, data, data_bytes);
        memcpy(msg->msg_iov[3].iov_base, u32intToBits(data_bytes), INT_SIZE);
    }
    else
        strcpy(msg->msg_iov[2].iov_base, data);

    return;
}

void freeMsghdr(struct msghdr *msg)
{
    free(msg->msg_iov[0].iov_base);
    free(msg->msg_iov[1].iov_base);
    free(msg->msg_iov[2].iov_base);
    free(msg->msg_iov[3].iov_base);
    free(msg->msg_iov);
    free(msg);
    return;
}

int readable_timeo(int fd, int usec)
{
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    tv.tv_sec = 0;
    tv.tv_usec = usec;

    return(select(fd + 1, &rset, NULL, NULL, &tv));// > 0 if fd is readable
}
