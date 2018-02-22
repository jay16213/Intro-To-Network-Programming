#include "headers.h"

void recvFileInfo(
    int sockfd,
    struct sockaddr_in *sender_addr,
    char *filename,
    uint32_t *fileSize,
    uint32_t *numOfPkts
    )
{
    struct msghdr *sendbuf, *recvbuf;
    sendbuf = getEmptyBuf(sender_addr, sizeof(struct sockaddr_in));
    recvbuf = getEmptyBuf(sender_addr, sizeof(struct sockaddr_in));

    uint32_t rn = 0;//request num
    printf("waiting for new sender...\n");
    while(rn < 2)
    {
        ssize_t nRead;
        if(readable_timeo(sockfd, RECV_TIMEO_USEC) == 0)
        {
            if(rn) printf("recv timeout\n");
            continue;
        }
        else
        {
            if((nRead = recvmsg(sockfd, recvbuf, 0)) < 0)
            {
                perror("recv error");
                exit(-1);
            }
        }

        printf("recv fileinfo\n");
        printf("type: %s\n", (char *) recvbuf->msg_iov[0].iov_base);
        printf("sn: %u\n", bitsTou32int((char *) recvbuf->msg_iov[1].iov_base));

        char type = *((char *) recvbuf->msg_iov[0].iov_base);
        switch(type)
        {
            case FILENAME:
                if(rn == 1) break;
                strcpy(filename, (char *) recvbuf->msg_iov[2].iov_base);
                printf("filename: %s\n", filename);
                rn++;
                break;

            case FILESIZE:
                *fileSize = bitsTou32int((char *) recvbuf->msg_iov[2].iov_base);
                *numOfPkts = 2 + (*fileSize / SEG_SIZE);
                if(*fileSize % SEG_SIZE != 0) (*numOfPkts)++;

                printf("filesize: %u\n", *fileSize);
                printf("numofPkts: %u\n", *numOfPkts);
                rn++;
                break;
        }

        setMsghdr(sendbuf, ACK, rn, "a", 2);

        printf("send ack rn: %u\n", bitsTou32int((char *) sendbuf->msg_iov[1].iov_base));

        ssize_t nWritten = sendmsg(sockfd, sendbuf, 0);
        if(nWritten < 0)
        {
            perror("send error");
            exit(-1);
        }
        printf("==========fileinfo==========\n");
    }
}

void outputFile(const char *filename, char **buf, uint32_t *buflen, uint32_t numOfPkts)
{
    FILE *fp = fopen(filename, "wb");

    int i;
    for(i = DATA_BEGIN; i < numOfPkts; i++)
    {
        size_t nwritten = fwrite(buf[i], 1, buflen[i], fp);
        if(nwritten < 0)
        {
            perror("fwrite error");
            exit(-1);
        }
    }

    fclose(fp);
    return;
}

void receiver(int sockfd, struct sockaddr_in *sender_addr)
{
    char filename[SEG_SIZE];
    uint32_t fileSize, numOfPkts;

    printf("=======================\n");
    recvFileInfo(sockfd, sender_addr, filename, &fileSize, &numOfPkts);

    char **fileBuf = (char **) malloc(numOfPkts * sizeof(char *));
    uint32_t *fileBuf_len = (uint32_t *) malloc(numOfPkts * sizeof(uint32_t));
    uint32_t *seqTable = (uint32_t *) malloc(numOfPkts * sizeof(uint32_t));

    int i;
    for(i = 0; i < numOfPkts; i++)
    {
        seqTable[i] = 0;
        fileBuf[i] = (char *) malloc(SEG_SIZE * sizeof(char));
        memset(fileBuf[i], 0, SEG_SIZE);
    }

    struct msghdr *sendbuf, *recvbuf;
    sendbuf = getEmptyBuf(sender_addr, sizeof(struct sockaddr_in));
    recvbuf = getEmptyBuf(sender_addr, sizeof(struct sockaddr_in));

    uint32_t rn = 2;
    int finish_transfer = 0;
    while(rn < numOfPkts)
    {
        ssize_t nRead;

        if(readable_timeo(sockfd, RECV_TIMEO_USEC) == 0)
        {
            printf("recv timeout\n");
            goto SEND_ACK;
        }
        else
        {
            if((nRead = recvmsg(sockfd, recvbuf, 0)) < 0)
            {
                perror("recv error");
                exit(-1);
            }
        }

        printf("sn: %u\n", bitsTou32int((char *) recvbuf->msg_iov[1].iov_base));

        uint32_t sn = bitsTou32int((char *) recvbuf->msg_iov[1].iov_base);
        if(seqTable[sn] == 0)
        {
            seqTable[sn] = 1;
            fileBuf_len[sn] = bitsTou32int((char *) recvbuf->msg_iov[3].iov_base);
            memcpy(fileBuf[sn], (char *) recvbuf->msg_iov[2].iov_base, fileBuf_len[sn]);
        }

        while(seqTable[rn] == 1) rn++;

        SEND_ACK:
            setMsghdr(sendbuf, ACK, rn, "ack", sizeof("ack"));
            ssize_t nWritten = sendmsg(sockfd, sendbuf, 0);
            if(nWritten < 0)
            {
                perror("send error");
                exit(-1);
            }
    }

    //try to recv sender's close msg & close connection
    //re-transmit ack at most 20 times if packet loss
    for(i = 0; i < 20; i++)
    {
        printf("==========close==========\n");
        if(readable_timeo(sockfd, RECV_TIMEO_USEC) == 0)
        {
            printf("recv timeout\n");
            continue;
        }
        else
        {
            if(recvmsg(sockfd, recvbuf, 0) < 0)
            {
                perror("recv error");
                exit(-1);
            }
        }

        if(strcmp((char *) recvbuf->msg_iov[2].iov_base, "close") == 0)
        {
            printf("close connection\n");
            break;
        }

        printf("send ack %u\n", bitsTou32int((char *) sendbuf->msg_iov[1].iov_base));
        ssize_t nWritten = sendmsg(sockfd, sendbuf, 0);
        if(nWritten < 0)
        {
            perror("send error");
            exit(-1);
        }
    }

    outputFile(filename, fileBuf, fileBuf_len, numOfPkts);

    //free malloc mem
    freeMsghdr(sendbuf);
    freeMsghdr(recvbuf);
    free(seqTable);
    free(fileBuf_len);
    for(i = 0; i < numOfPkts; i++) free(fileBuf[i]);
    free(fileBuf);
    return;
}
