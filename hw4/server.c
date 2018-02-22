#include "headers.h"

int stablishConnect(int listenfd)
{
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    int connfd;
    if((connfd = accept(listenfd, (struct sockaddr *) &cli_addr, &cli_len)) < 0)
    {
        perror("Accept error");
        return -1;
    }
    printf("New connect from: %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

    int flags = fcntl(connfd, F_GETFL, 0);
    fcntl(connfd, F_SETFL, flags | O_NONBLOCK);
    return connfd;
}

void syncFileRequest(struct Client *cli)
{
    int i;
    for(i = 0; i < numOfFiles; i++)
    {
        if(strcmp(fileList[i].username, cli->name) == 0)
        {
            char buf[CMD_SIZE];
            snprintf(buf, CMD_SIZE, "/get %s", fileList[i].filename);
            memcpy(buf + 261, &fileList[i].fileSize, 4);
            memcpy(cli->cmdsiptr, buf, CMD_SIZE);//copy to cmdbuf
            cli->cmdsiptr += CMD_SIZE;
        }
    }
    return;
}

void addFile(char *username, char *filename, uint32_t fileSize)
{
    if(numOfFiles >= FILELIST_SIZE)
    {
        printf("Error: FileList overflow.\n");
        exit(-1);
    }

    int i;
    for(i = 0; i < numOfFiles; i++)
    {
        //if the user already has same name file => don't add to list
        if(strcmp(fileList[i].username, username) == 0 &&
           strcmp(fileList[i].filename, filename) == 0)
            return;
    }

    fileList[numOfFiles].username = (char *) malloc(NAME_SIZE * sizeof(char));
    fileList[numOfFiles].filename = (char *) malloc(NAME_SIZE * sizeof(char));
    strcpy(fileList[numOfFiles].username, username);
    strcpy(fileList[numOfFiles].filename, filename);
    fileList[numOfFiles].fileSize = fileSize;
    numOfFiles++;
    return;
}

void server(int listenfd)
{
    fd_set rset, wset;
    int maxfd = listenfd + 1;
    int i;

    ssize_t nRead, nWritten;
    while(1)
    {
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        FD_SET(listenfd, &rset);

        for(i = 0; i < CLI_SIZE; i++)
        {
            if(client[i].cmdLink >= 0)
            {
                if((client[i].cmdsiptr - client[i].cmdsoptr) >= CMD_SIZE)
                    FD_SET(client[i].cmdLink, &wset);

                if((&client[i].cmdrbuf[CMDLINE] - client[i].cmdriptr) >= CMD_SIZE)
                    FD_SET(client[i].cmdLink, &rset);
            }

            if(client[i].dataLink >= 0)
            {
                if((client[i].sendiptr - client[i].sendoptr) >= PKT_SIZE)
                    FD_SET(client[i].dataLink, &wset);

                if((&client[i].recvbuf[DATALINE] - client[i].recviptr) >= PKT_SIZE)
                    FD_SET(client[i].dataLink, &rset);
            }
        }

        for(i = 0; i < TMP_SIZE; i++)
        {
            if(tmpfd[i].fd >= 0)
            {
                FD_SET(tmpfd[i].fd, &rset);
                if(!tmpfd[i].sent) FD_SET(tmpfd[i].fd, &wset);
            }
        }

        Select(maxfd + 1, &rset, &wset, NULL, NULL);

        if(FD_ISSET(listenfd, &rset))
        {
            int connfd = stablishConnect(listenfd);
            if(connfd >= 0)
            {
                for(i = 0; i < TMP_SIZE; i++)
                {
                    if(tmpfd[i].fd < 0)
                    {
                        tmpfd[i].fd = connfd;
                        tmpfd[i].sent = 0;
                        break;
                    }
                }
            }

            if(connfd > maxfd) maxfd = connfd;
            FD_SET(connfd, &rset);
        }

        for(i = 0; i < TMP_SIZE; i++)
        {
            if(tmpfd[i].fd < 0) continue;

            char buf[PKT_SIZE];
            if(!tmpfd[i].sent && FD_ISSET(tmpfd[i].fd, &wset))
            {
                size_t n = nonBlockWrite(tmpfd[i].fd, &tmpfd[i].fd, 4);
                if(n > 0) tmpfd[i].sent = 1;
            }

            if(FD_ISSET(tmpfd[i].fd, &rset))
            {
                ssize_t n = nonBlockRead(tmpfd[i].fd, buf, PKT_SIZE);
                if(n < 0) continue;
                buf[n] = '\0';

                int cmdLink, dataLink;
                char username[NAME_SIZE];
                memcpy(&cmdLink, buf, 4);
                memcpy(&dataLink, buf + 4, 4);
                strcpy(username, buf + 8);

                int j;
                for(j = 0; j < CLI_SIZE; j++)
                {
                    if(!client[j].occupied)
                    {
                        client[j].occupied = 1;
                        client[j].flags = C_CONNECTING;
                        newClient(&client[j], cmdLink, dataLink, username);
                        fprintf(stdout, "New Client, username: %s %d %d\n", client[j].name, client[j].cmdLink, client[j].dataLink);

                        syncFileRequest(&client[j]);

                        snprintf(client[j].sendbuf, PKT_SIZE, "Welcome to the dropbox-like server!: %s\n", client[j].name);
                        client[j].sendiptr += PKT_SIZE;
                        FD_SET(client[j].dataLink, &wset);
                        break;
                    }
                }

                for(j = 0; j < TMP_SIZE; j++)
                {
                    if(tmpfd[j].fd == cmdLink) tmpfd[j].fd = -1;
                    if(tmpfd[j].fd == dataLink) tmpfd[j].fd = -1;
                }
            }
        }

        for(i = 0; i < CLI_SIZE; i++)
        {
            if(!client[i].occupied) continue;

            //write get request to client(download only)
            if(FD_ISSET(client[i].cmdLink, &wset) && (client[i].cmdsiptr - client[i].cmdsoptr >= CMD_SIZE))
            {
                nWritten = nonBlockWrite(client[i].cmdLink, client[i].cmdsoptr, CMD_SIZE);
                if(nWritten > 0)
                {
                    client[i].cmdsoptr += CMD_SIZE;
                    if(client[i].cmdsoptr == client[i].cmdsiptr)
                        client[i].cmdsoptr = client[i].cmdsiptr = client[i].cmdsbuf;
                    FD_SET(client[i].cmdLink, &rset);
                }
            }

            if(FD_ISSET(client[i].cmdLink, &rset) && (&client[i].cmdrbuf[CMDLINE] - client[i].cmdriptr) >= CMD_SIZE)
            {
                nRead = nonBlockRead(client[i].cmdLink, client[i].cmdriptr, CMD_SIZE);
                if(nRead == 0)
                {
                    close(client[i].cmdLink);
                    FD_CLR(client[i].cmdLink, &rset);
                    FD_CLR(client[i].cmdLink, &wset);
                    client[i].cmdLink = -1;
                    if(client[i].dataLink == -1) freeClient(&client[i]);
                    continue;
                }
                if(nRead > 0)
                {
                    client[i].cmdQueue++;
                    client[i].cmdriptr += CMD_SIZE;
                }
            }

            //client is IDLE => take a job(upload/download)
            if((client[i].flags & C_IDLE) && (client[i].cmdQueue > 0))
            {
                char buf[CMD_SIZE];
                memcpy(buf, client[i].cmdroptr, CMD_SIZE);

                char *cmd = strtok(buf, DELI);
                if(cmd == NULL)
                {
                    printf("Invalid command.\n");
                    goto RECVCMD_DONE;
                }

                if(strcmp(cmd, "/put") == 0)
                {
                    client[i].cmdQueue--;
                    client[i].flags = C_UPLOADING;

                    char *filename = strtok(NULL, DELI);
                    strcpy(client[i].filename, filename);
                    char filename_s[2*NAME_SIZE];
                    snprintf(filename_s, 2*NAME_SIZE, "%s.%s", client[i].filename, client[i].name);
                    client[i].fp = Fopen(filename_s, "wb");
                    client[i].fileSize = 0;

                    client[i].sn = 0;
                    char ack = ACK;
                    memcpy(client[i].sendiptr, &ack, 1);
                    memcpy(client[i].sendiptr + 1, &(client[i].sn), 4);
                    client[i].sendiptr += PKT_SIZE;
                    FD_SET(client[i].dataLink, &wset);
                }
                else if(strcmp(cmd, "/get") == 0)
                {
                    client[i].cmdQueue--;
                    client[i].flags = C_DOWNLOADING;

                    char *filename = strtok(NULL, DELI);
                    strcpy(client[i].filename, filename);
                    char filename_s[2*NAME_SIZE];
                    snprintf(filename_s, 2*NAME_SIZE, "%s.%s", client[i].filename, client[i].name);
                    client[i].fp = Fopen(filename_s, "rb");

                    client[i].sn = -1;
                    client[i].final = DATA;
                }

                RECVCMD_DONE:
                {
                    client[i].cmdroptr += CMD_SIZE;
                    if(client[i].cmdroptr == client[i].cmdriptr)
                        client[i].cmdroptr = client[i].cmdriptr = client[i].cmdrbuf;
                }
            }

            if(FD_ISSET(client[i].dataLink, &rset) && (&client[i].recvbuf[DATALINE] - client[i].recviptr) >= PKT_SIZE)
            {
                nRead = nonBlockRead(client[i].dataLink, client[i].recviptr, PKT_SIZE);
                if(nRead == 0)
                {
                    close(client[i].dataLink);
                    FD_CLR(client[i].dataLink, &rset);
                    FD_CLR(client[i].dataLink, &wset);
                    client[i].dataLink = -1;
                    if(client[i].cmdLink == -1) freeClient(&client[i]);
                    continue;
                }

                if(nRead > 0) client[i].recviptr += PKT_SIZE;
            }

            //send welcome msg / ack
            if(FD_ISSET(client[i].dataLink, &wset) && ((client[i].sendiptr - client[i].sendoptr) >= PKT_SIZE))
            {
                nWritten = nonBlockWrite(client[i].dataLink, client[i].sendoptr, PKT_SIZE);
                if(nWritten > 0)
                {
                    client[i].sendoptr += PKT_SIZE;
                    if(client[i].sendoptr == client[i].sendiptr)
                        client[i].sendoptr = client[i].sendiptr = client[i].sendbuf;

                    if(client[i].flags & C_CONNECTING) client[i].flags = C_IDLE;
                }
            }

            //read from file
            if((client[i].flags & C_DOWNLOADING) && ((client[i].recviptr - client[i].recvoptr) >= PKT_SIZE))
            {
                uint32_t rn;
                memcpy(&rn, client[i].recvoptr + 1, 4);
                client[i].recvoptr += PKT_SIZE;
                if(client[i].recvoptr == client[i].recviptr)
                    client[i].recvoptr = client[i].recviptr = client[i].recvbuf;

                if(rn == client[i].sn + 1)
                {
                    if(client[i].final == F_DATA)
                    {
                        fprintf(stdout, "%s download %s complete!\n", client[i].name, client[i].filename);
                        closeFile(&client[i]);
                    }
                    else
                    {
                        client[i].sn++;

                        size_t n;
                        char data[SEG_SIZE];
                        if((n = fread(data, 1, SEG_SIZE, client[i].fp)) >= 0)
                        {
                            client[i].final = feof(client[i].fp) ? F_DATA : DATA;

                            memcpy(client[i].sendiptr, &client[i].final, 1);
                            memcpy(client[i].sendiptr + 1, &client[i].sn, 4);
                            memcpy(client[i].sendiptr + 5, &n, 4);
                            memcpy(client[i].sendiptr + 9, data, n);

                            client[i].sendiptr += PKT_SIZE;
                        }
                    }
                }
            }

            //write to file
            if((client[i].flags & C_UPLOADING) && ((client[i].recviptr - client[i].recvoptr) >= PKT_SIZE))
            {
                size_t n;
                char final, data[SEG_SIZE];
                uint32_t sn, bytes;

                memcpy(&final, client[i].recvoptr, 1);
                memcpy(&sn, client[i].recvoptr + 1, 4);
                memcpy(&bytes, client[i].recvoptr + 5, 4);
                memcpy(&data, client[i].recvoptr + 9, bytes);

                //wrong pkt => ignore this pkt
                if(sn != client[i].sn) goto DONE;

                if((n = fwrite(data, 1, bytes, client[i].fp)) < 0)
                {
                    perror("fwrite error");
                    exit(-1);
                }

                client[i].fileSize += n;
                client[i].sn++;

                if(final & F_DATA)//final data
                {
                    fprintf(stdout, "%s upload %s complete!\n", client[i].name, client[i].filename);

                    char buf[CMD_SIZE];
                    snprintf(buf, CMD_SIZE, "/get %s", client[i].filename);
                    memcpy(buf + 261, &client[i].fileSize, 4);

                    addFile(client[i].name, client[i].filename, client[i].fileSize);

                    int j;
                    for(j = 0; j < CLI_SIZE; j++)
                    {
                        if(!client[j].occupied) continue;
                        if(i == j) continue;//don't transfer to myself

                        if(strcmp(client[i].name, client[j].name) == 0)
                        {
                            if((&client[j].cmdsbuf[CMDLINE] - client[j].cmdsiptr) < CMD_SIZE)
                            {
                                printf("Error: command buffer overflow.\n");
                                exit(-1);
                            }
                            memcpy(client[j].cmdsiptr, buf, CMD_SIZE);
                            client[j].cmdsiptr += CMD_SIZE;
                        }
                    }

                    closeFile(&client[i]);
                }

                DONE:
                {
                    client[i].recvoptr += PKT_SIZE;
                    if(client[i].recvoptr == client[i].recviptr)
                        client[i].recvoptr = client[i].recviptr = client[i].recvbuf;

                    char ack = ACK;
                    memcpy(client[i].sendiptr, &ack, 1);
                    memcpy(client[i].sendiptr + 1, &client[i].sn, 4);
                    client[i].sendiptr += PKT_SIZE;
                }
            }
        }
    }

    return;
}

int main(int argc, char  **argv)
{
    if(argc < 2)
    {
        printf("Usage: ./server <port>\n");
        exit(-1);
    }

    int listenfd;
    struct sockaddr_in server_addr;
    char ip[INET_ADDRSTRLEN];

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    if(bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind error");
        exit(-1);
    }

    if(listen(listenfd, LISTENQ) < 0)
    {
        perror("Listen error");
        exit(-1);
    }

    inet_ntop(AF_INET, &(server_addr.sin_addr), ip, INET_ADDRSTRLEN);
    printf("Server is listening on %s:%s\n", ip, argv[1]);

    int flags;

    flags = fcntl(listenfd, F_GETFL, 0);
    fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);

    int i;
    for(i = 0; i < CLI_SIZE; i++)
    {
        client[i].occupied = 0;
        client[i].cmdLink = -1;
        client[i].dataLink = -1;
        client[i].cmdQueue = 0;
        client[i].fileSize = 0;
    }

    for(i = 0; i < TMP_SIZE; i++) tmpfd[i].fd = -1;

    numOfFiles = 0;
    server(listenfd);

    for(i = 0; i < numOfFiles; i++)
    {
        free(fileList[i].username);
        free(fileList[i].filename);
    }
    return 0;
}
