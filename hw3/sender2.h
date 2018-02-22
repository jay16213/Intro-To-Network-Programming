#include "headers.h"

char** readFile(const char *filename, uint32_t *fileSize, uint32_t *numOfPkts, uint32_t **sendBytes)
{
    FILE *fp = fopen(filename, "rb");//open file in binary mode
    if(fp == NULL)
    {
        perror("Can not open the file");
        return NULL;
    }

    fseek(fp, 0, SEEK_END);//jump to the end of file
    *fileSize = ftell(fp);  //get current byte offset in the file
    rewind(fp);            //jump back to the beginning of the file
    printf("file size: %d\n", *fileSize);

    *numOfPkts = (*fileSize / SEG_SIZE) + 2;//+2: filename & filesize
    if(*fileSize % SEG_SIZE != 0) (*numOfPkts)++;
    printf("num of pkts: %u\n", *numOfPkts);


    *sendBytes = (uint32_t *) malloc(*numOfPkts * sizeof(uint32_t));
    char **sendData = (char **) malloc(*numOfPkts * sizeof(char *));

    sendData[0] = (char *) malloc(SEG_SIZE * sizeof(char));
    strncpy(sendData[0], filename, strlen(filename));
    printf("filename: %s\n", sendData[0]);

    sendData[1] = u32intToBits(*fileSize);
    printf("filesize: %u\n", bitsTou32int(sendData[1]));

    int i;
    for(i = DATA_BEGIN; i < *numOfPkts; i++)
    {
        sendData[i] = (char *) malloc(SEG_SIZE * sizeof(char));
        memset(sendData[i], 0, SEG_SIZE);
        size_t n = fread(sendData[i], 1, SEG_SIZE, fp);
        if(n < 0)
        {
            perror("fread error");
            exit(-1);
        }

        (*sendBytes)[i] = n;
    }
    fclose(fp);
    return sendData;
}

void sendFileInfo(int sockfd, struct sockaddr_in *recv_addr, socklen_t recvlen, char **sendData)
{
    struct msghdr *sendbuf[2], *recvbuf;
    sendbuf[0] = getEmptyBuf(recv_addr, recvlen);
    setMsghdr(sendbuf[0], FILENAME, 0, sendData[0], sizeof(sendData[0]));

    sendbuf[1] = getEmptyBuf(recv_addr, recvlen);
    setMsghdr(sendbuf[1], FILESIZE, 1, sendData[1], sizeof(sendData[1]));

    recvbuf = getEmptyBuf(recv_addr, recvlen);

    uint32_t sn = 0;
    while(sn < 2)
    {
        printf("==========fileinfo==========\n");
        printf("type: %s\n", (char *) sendbuf[sn]->msg_iov[0].iov_base);
        printf("sn: %u\n", bitsTou32int((char *) sendbuf[sn]->msg_iov[1].iov_base));
        printf("data: %s\n", (char *) sendbuf[sn]->msg_iov[2].iov_base);

        if(sendmsg(sockfd, sendbuf[sn], 0) < 0)
        {
            perror("send error");
            exit(-1);
        }

        ualarm(SEND_TIMEO_USEC, 0);
        if(recvmsg(sockfd, recvbuf, 0) < 0)
        {
            if(errno == EINTR)
            {
                printf("recv ack timeout\n");
                continue;
            }
            else
            {
                perror("recv error");
                exit(-1);
            }
        }
        ualarm(0, 0);

        printf("recv ack rn: %u\n", bitsTou32int((char *) recvbuf->msg_iov[1].iov_base));

        uint32_t rn = bitsTou32int(recvbuf->msg_iov[1].iov_base);
        if(rn > sn) sn++;
    }

    freeMsghdr(sendbuf[0]);
    freeMsghdr(sendbuf[1]);
    freeMsghdr(recvbuf);
    return;
}

void sendFile(int sockfd, struct sockaddr_in *recv_addr, socklen_t recvlen, const char *filename)
{
    uint32_t fileSize, numOfPkts, *sendBytes;
    char **sendData;

    sendData = readFile(filename, &fileSize, &numOfPkts, &sendBytes);
    printf("===========================\n");

    sendFileInfo(sockfd, recv_addr, recvlen, sendData);

    struct msghdr *sendbuf, *recvbuf;
    sendbuf = getEmptyBuf(recv_addr, recvlen);
    recvbuf = getEmptyBuf(recv_addr, recvlen);

    uint32_t sn = DATA_BEGIN;
    while(sn < numOfPkts)
    {
        setMsghdr(sendbuf, DATA, sn, sendData[sn], sendBytes[sn]);

        printf("sn: %u\n", bitsTou32int((char *) sendbuf->msg_iov[1].iov_base));

        ssize_t nWritten = sendmsg(sockfd, sendbuf, 0);
        if(nWritten < 0)
        {
            perror("send error");
            exit(-1);
        }

        ualarm(SEND_TIMEO_USEC, 0);
        ssize_t nRead = recvmsg(sockfd, recvbuf, 0);
        if(nRead < 0)
        {
            if(errno == EINTR)
            {
                printf("recv ack timeout\n");
                continue;
            }
            else
            {
                perror("recvmsg error");
                exit(-1);
            }
        }
        ualarm(0, 0);

        uint32_t rn = bitsTou32int((char *) recvbuf->msg_iov[1].iov_base);
        if(rn > sn) sn = rn;
    }

    setMsghdr(sendbuf, ACK, sn, "close", sizeof("close"));
    printf("**********send close**********\n");
    if(sendmsg(sockfd, sendbuf, 0) < 0)
    {
        perror("send error");
        exit(-1);
    }

    freeMsghdr(sendbuf);
    freeMsghdr(recvbuf);
    for(sn = 0; sn < numOfPkts; sn++) free(sendData[sn]);
    free(sendData);
    free(sendBytes);
    return;
}
