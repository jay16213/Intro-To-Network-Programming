#include "headers.h"

static const char ack = ACK;

void doSleep(int sec)
{
    fprintf(stdout, "Client starts to sleep\n");
    int i;
    for(i = 1; i <= sec; i++)
    {
        sleep(1);
        fprintf(stdout, "Sleep %d\n", i);
    }
    printf("Client wakes up\n");
}

void cli(int cmdLink, int dataLink)
{
    int maxfd;
    if(cmdLink < dataLink) maxfd = dataLink + 1;
    else maxfd = cmdLink + 1;

    int disconnect = 0;
    int state = C_CONNECTING;
    fd_set rset;
    ssize_t nRead, nWritten;
    char cmdbuf[CMDLINE], sendbuf[DATALINE], recvbuf[DATALINE];
    FILE *fp;

    int sn;
    char final;
    char filename[256];
    uint32_t fileSize;
    int barFin;
    while(1)
    {
        FD_ZERO(&rset);
        if(!disconnect) FD_SET(STDIN_FILENO, &rset);
        if(cmdLink >= 0) FD_SET(cmdLink, &rset);
        if(dataLink >= 0) FD_SET(dataLink, &rset);

        memset(sendbuf, 0, DATALINE);

        Select(maxfd, &rset, NULL, NULL, NULL);

        if((state & C_IDLE) && FD_ISSET(STDIN_FILENO, &rset))//user cmd
        {
            char c;
            int index = 0;
            for(;index < CMDLINE; index++)
            {
                c = getchar();
                recvbuf[index] = c;
                if(c == '\n') break;
                else if(c == 0)
                    recvbuf[index] = '\0';
            }
            //read input
            char tmp[CMDLINE];
            strcpy(tmp, recvbuf);
            char *cmd = strtok(tmp, DELI);

            if(cmd != NULL && strcmp(cmd, "/exit") == 0)
            {
                disconnect = 1;
                shutdown(cmdLink, SHUT_WR);
                shutdown(dataLink, SHUT_WR);
                continue;
            }
            else if(cmd != NULL && strcmp(cmd, "/sleep") == 0)
            {
                char *t = strtok(NULL, DELI);
                if(t == NULL)
                    printf("You should input the seconds you want to sleep.\n");
                else
                    doSleep(atoi(t));
            }
            else if(cmd != NULL && strcmp(cmd, "/put") == 0)
            {
                char *f = strtok(NULL, DELI);
                if(f == NULL)
                    printf("You should input a filename.\n");
                else
                {
                    strcpy(filename, f);
                    fp = fopen(filename, "rb");
                    if(fp == NULL)
                    {
                        perror("fopen error");
                        continue;
                    }
                    fileSize = getFileSize(fp);
                    sn = -1;
                    final = 0;
                    barFin = 0;
                    state = C_UPLOADING;
                    Writen(cmdLink, recvbuf, CMD_SIZE);//send put cmd
                    fprintf(stdout, "Uploading file : %s\n", filename);
                }
            }
            else
                printf("Error command.\n");
        }

        if((state & C_IDLE) && FD_ISSET(cmdLink, &rset))//server request
        {
            nRead = read(cmdLink, cmdbuf, CMD_SIZE);
            if(nRead == 0 && disconnect)
            {
                printf("Close cmd connection.\n");
                cmdLink = -1;
                disconnect++;
                if(disconnect == 3) return;
                else continue;
            }
            cmdbuf[nRead] = '\0';

            char buf[CMD_SIZE];
            memcpy(buf, cmdbuf, CMD_SIZE - 4);
            char *cmd = strtok(cmdbuf, DELI);
            if(strcmp(cmd, "/get") == 0)
            {
                Writen(cmdLink, buf, CMD_SIZE - 4);
                char *f = strtok(NULL, DELI);
                strcpy(filename, f);
                fp = Fopen(filename, "wb");
                memcpy(&fileSize, cmdbuf + 261, 4);

                barFin = 0;
                state = C_DOWNLOADING;

                sn = 0;
                memcpy(sendbuf, &ack, 1);  //pkt type: ack
                memcpy(sendbuf + 1, &sn, 4);//pkt sn: 0
                Writen(dataLink, sendbuf, PKT_SIZE);
                fprintf(stdout, "Downloading file : %s\n", filename);
            }
        }

        if(FD_ISSET(dataLink, &rset))//upload/download files
        {
            nRead = Read(dataLink, recvbuf, PKT_SIZE);
            if(nRead == 0 && disconnect)
            {
                printf("Close dataLink\n");
                dataLink = -1;
                disconnect++;
                if(disconnect == 3) return;
                else continue;
            }

            switch(state)
            {
                case C_UPLOADING:
                {
                    uint32_t rn;
                    memcpy(&rn, recvbuf + 1, 4);

                    if(rn == sn + 1)
                    {
                        if(final)
                        {
                            fclose(fp);
                            fp = NULL;
                            state = C_IDLE;
                            printf("Upload %s complete!\n", filename);
                            continue;
                        }

                        sn++;
                        if(!barFin) barFin = printProgress((sn + 1) * SEG_SIZE, fileSize);

                        char data[SEG_SIZE];
                        size_t n;
                        if((n = fread(data, 1, SEG_SIZE, fp)) >= 0)
                        {
                            sendbuf[0] = feof(fp) ? F_DATA : DATA;
                            if(sendbuf[0] == F_DATA) final = F_DATA;
                            memcpy(sendbuf + 1, &sn, 4); //seq
                            memcpy(sendbuf + 5, &n, 4);  //bytes
                            memcpy(sendbuf + 9, data, n);//data

                            Writen(dataLink, sendbuf, PKT_SIZE);
                        }
                    }
                    break;
                }

                case C_DOWNLOADING:
                {
                    char rfinal, data[SEG_SIZE];
                    uint32_t rn, bytes;

                    memcpy(&rfinal, recvbuf, 1);
                    memcpy(&rn, recvbuf + 1, 4);
                    memcpy(&bytes, recvbuf + 5, 4);
                    memcpy(data, recvbuf + 9, bytes);

                    if(rn != sn) goto DONE;

                    if(fwrite(data, 1, bytes, fp) < 0)
                    {
                        perror("fwrite error");
                        exit(-1);
                    }

                    if(bytes) printProgress((sn + 1) * SEG_SIZE, fileSize);
                    sn++;

                    if(rfinal & F_DATA)
                    {
                        printf("Download %s complete!\n", filename);
                        fclose(fp);
                        fp = NULL;
                        memset(filename, 0, NAME_SIZE);
                        state = C_IDLE;
                    }

                    DONE:
                    {
                        memcpy(sendbuf, &ack, 1);
                        memcpy(sendbuf + 1, &sn, 4);
                        Writen(dataLink, sendbuf, PKT_SIZE);
                    }
                    break;
                }

                case C_CONNECTING:
                {
                    recvbuf[nRead] = '\0';
                    fputs(recvbuf, stdout);
                    state = C_IDLE;
                    break;
                }
            }
        }
    }

    return;
}

int main(int argc, char **argv)
{
    if(argc < 4)
    {
        printf("Usage: ./client <ip> <port> <username>.\n");
        exit(-1);
    }

    int cmdLink, dataLink;
    struct addrinfo hints, *servinfo;
    struct sockaddr_in server_addr;
    char *server_ip;

    cmdLink = socket(AF_INET, SOCK_STREAM, 0);
    dataLink = socket(AF_INET, SOCK_STREAM, 0);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if(getaddrinfo(argv[1], argv[2], &hints, &servinfo))
    {
        perror("Get addr info error");
        exit(-1);
    }

    server_ip = inet_ntoa(((struct sockaddr_in *) servinfo->ai_addr)->sin_addr);
    printf("Connecting to %s:%s...\n", server_ip, argv[2]);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));

    char *connbuf = (char *) malloc(CMDLINE * sizeof(char));
    if(connect(cmdLink, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
    {
        perror("Connect error");
        exit(-1);
    }
    ssize_t n = Read(cmdLink, connbuf, 4);//read remote cmdLink fd

    if(connect(dataLink, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
    {
        perror("Connect error");
        exit(-1);
    }
    n = Read(dataLink, connbuf + 4, 4);

    memcpy(connbuf + 8, argv[3], strlen(argv[3]));
    Writen(cmdLink, connbuf, 8 + strlen(argv[3]));
    free(connbuf);

    cli(cmdLink, dataLink);
    return 0;
}
