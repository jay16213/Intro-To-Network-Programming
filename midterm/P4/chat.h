#include "header.h"

#define LISTENQ 32
#define MAXCLIENT 32

struct Friend{
    char name[50];
    int fd;
    int get_name;
};

void broadcast(char *me, struct Friend *cli, char *msg)
{
    int i;
    for(i = 0; i < MAXCLIENT; i++)
    {
        if(cli[i].fd < 0) continue;
        if(strcmp(cli[i].name, me) == 0) continue;
        
        write(cli[i].fd, msg, strlen(msg));        
    }
}

int chat(char *me, struct Friend *cli, char *input)
{
    //fprintf(stdout, "cmd: %s", input);

    int i;
    char *cmd = strtok(input, TOK_DEM);

    if(strcmp(cmd, "EXIT") == 0)
    {
        char buf[100];
        snprintf(buf, 100, "%s has left the chat room.\n", me);
        broadcast(me, cli, buf);
        return 1;
    } 

    char *to_whom = cmd;
    char *msg = strtok(NULL, "\a\t\r\n");
    if(to_whom == NULL || msg == NULL)
    {
        perror("cmd error");
        exit(-1);
    }

    if(strcmp(to_whom, "ALL") == 0)
    {
        char buf[100];
        snprintf(buf, 100, "%s SAID %s\n", me, msg);
        broadcast(me, cli, buf);
        return 0;
    }
    else
    {
        for(i = 0; i < MAXCLIENT; i++)
        {
            if(cli[i].fd < 0) continue;
            if(strcmp(cli[i].name, to_whom) == 0)
            {
                char buf[100];
                snprintf(buf, 100, "%s SAID %s\n", me, msg);
                write(cli[i].fd, buf, strlen(buf));
                break;
            }
        }
    }
    return 0;
}