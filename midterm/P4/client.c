#include "chat.h"



int main(int argc, char **argv)
{
    if(argc < 3)
    {
        printf("Usage: ./client <Name> <myport> [port].\n");
        exit(0);
    }

    char my_name[50];
    strcpy(my_name, argv[1]);

    int listenfd;
    struct sockaddr_in server_addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[2]));

    bind(listenfd, (SA *) &server_addr, sizeof(server_addr));
    if(listen(listenfd, LISTENQ) == -1)
    {
        perror("listen error");
        exit(-1);
    }

    int connfd;
    struct sockaddr_in client_addr;
    socklen_t client_len;

    struct Friend con_cli[MAXCLIENT];
    int i, j;
    for(i = 0; i < MAXCLIENT; i++)
    {
        memset(con_cli[i].name, 0, 50);
        con_cli[i].fd = -1;
        con_cli[i].get_name = 0;
    }

    fd_set r_set, all_set;
    int maxfd = listenfd;

    FD_ZERO(&all_set);
    FD_SET(listenfd, &all_set);
    
    //connect myself
    connfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, "0.0.0.0", &server_addr.sin_addr);
    if(connect(connfd, (SA *) &server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect error");
        exit(-1);
    }
    con_cli[0].fd = connfd;
    con_cli[0].get_name = 1;
    strcpy(con_cli[0].name, my_name);
    FD_SET(connfd, &all_set);
    if(connfd > maxfd) maxfd = connfd;

    //connect to others
    for(j = 0, i = 3; j < MAXCLIENT && i < argc; j++)
    {
        if(con_cli[j].fd >= 0) continue;
        
        connfd = socket(AF_INET, SOCK_STREAM, 0);
        bzero(&server_addr, sizeof(server_addr));
        
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(atoi(argv[i++]));
        inet_pton(AF_INET, "0.0.0.0", &server_addr.sin_addr);

        if(connect(connfd, (SA *) &server_addr, sizeof(server_addr)) < 0)
        {
            perror("connect error");
            exit(-1);
        }
        con_cli[j].fd = connfd;
        FD_SET(connfd, &all_set);
        if(connfd > maxfd) maxfd = connfd;
    }

    FD_SET(fileno(stdin), &all_set);

    while(1)
    {
        r_set = all_set;
        int num_of_ready;

        if((num_of_ready = select(maxfd + 1, &r_set, NULL, NULL, NULL)) == -1)
        {
            perror("select error");
            exit(-1);
        }

        //new connect
        if(FD_ISSET(listenfd, &r_set))
        {
            ///fprintf(stdout, "ddd\n");
            client_len = sizeof(client_addr);
            if((connfd = accept(listenfd, (SA *) &client_addr, &client_len)) == -1)
            {
                perror("Accept error");
                exit(-1);
            }

            for(i = 0; i < MAXCLIENT; i++)
            {
                if(con_cli[i].fd < 0)
                {
                    con_cli[i].fd = connfd;
                    char buf[100];
                    snprintf(buf, 100, "name %s\n", my_name);
                    write(connfd, buf, strlen(buf));
                    break;
                }
            }

            if(i == MAXCLIENT)
                fprintf(stdout, "too many client.\n");

            FD_SET(connfd, &all_set);
            if(connfd > maxfd) maxfd = connfd;
        }

        //recv msg
        for(i = 0; i < MAXCLIENT; i++)
        {
            if(con_cli[i].fd < 0) continue;
            int ter = 0;
            if(FD_ISSET(con_cli[i].fd, &r_set))
            {
                char cmd[MAXLINE];
                int n = read(con_cli[i].fd, cmd, MAXLINE);
                if(n <= 0)
                {
                    close(con_cli[i].fd);
                    FD_CLR(con_cli[i].fd, &all_set);                
                    continue;
                }
    
                cmd[n] = '\0';
                char tmp[100];
                strcpy(tmp, cmd);
                char *c = strtok(tmp, TOK_DEM);
                if(strcmp(c, "name") == 0 && !con_cli[i].get_name)
                {
                    char *name = strtok(NULL, TOK_DEM);
                    strcpy(con_cli[i].name, name);
                    con_cli[i].get_name = 1;
                    char buf[100];
                    snprintf(buf, 100, "name %s\n", my_name);
                    write(con_cli[i].fd, buf, strlen(buf));
                }
                else
                {
                    if(strcmp(c, "name") != 0)
                        fputs(cmd, stdout);
                }
            }
        }

        //send msg
        if(FD_ISSET(fileno(stdin), &r_set))
        {
            char input[MAXLINE];
            if(fgets(input, MAXLINE, stdin) == NULL) break;

            int ter = chat(my_name, con_cli, input);
            if(ter == 1) exit(0);
        }
    }
    return 0;
}