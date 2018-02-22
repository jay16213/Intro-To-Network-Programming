#ifndef UNP_H
#define UNP_H

#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUF_SIZE 8192
#define LISTENQ 32
#define FD_SIZE 128
#define TOK_DELIMETER " \a\t\r\n"

int Getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
    int err;
    if((err = getaddrinfo(node, service, hints, res)) != 0)
    {
        perror("Getaddrinfo error");
        exit(EXIT_FAILURE);
    }
    return err;
}

int Socket(int domain, int type, int protocol)
{
    int fd;
    if((fd = socket(domain, type, protocol)) == -1)
    {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }
    return fd;
}

int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if(bind(sockfd, addr, addrlen) == -1)
    {
        perror("Bind error");
        exit(EXIT_FAILURE);
    }
    return 0;
}

int Listen(int sockfd, int backlog)
{
    if(listen(sockfd, backlog) == -1)
    {
        perror("Listen error");
        exit(EXIT_FAILURE);
    }
    return 0;
}

const char *Inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if(inet_ntop(af, src, dst, size) == NULL)
    {
        perror("Inet_ntop error");
        exit(EXIT_FAILURE);
    }
    return dst;
}

int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    int n;
    if((n = select(nfds, readfds, writefds, exceptfds, timeout)) == -1)
    {
        perror("Select error");
        exit(EXIT_FAILURE);
    }
    return n;
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int connfd;
    if((connfd = accept(sockfd, addr, addrlen)) == -1)
    {
        perror("Accept error");
        exit(EXIT_FAILURE);
    }
    return connfd;
}

ssize_t Write(int fd, const void *buf, size_t count)
{
    ssize_t n;
    if((n = write(fd, buf, count)) == -1)
    {
        perror("Write error");
        return -1;
    }
    return n;
}

//write n bytes to a descriptor
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

struct Client {
    int occupied;
    int fd;
    struct sockaddr_in addr;
    char ip[INET_ADDRSTRLEN];
    int port;
    char name[13];
};

void setClient(struct Client *client, int fd, struct sockaddr_in addr, char *ip, int port)
{
    strcpy(client->name, "anonymous");
    client->fd = fd;
    client->addr = addr;
    strcpy(client->ip, ip);
    client->port = port;
    return;
}

int validUsername(char *name)
{
    int i;
    for(i = 0; i < strlen(name); i++)
        if(!isalpha(name[i])) return 0;

    return strlen(name) >= 2 && strlen(name) <= 12;
}

int commandHandler(int *b, char *input, struct Client client[], struct Client *sender,int *recevier, char *sender_msg, char *recevier_msg, char *broadcast)
{
    char *cmd = strtok(input, TOK_DELIMETER);
    if(cmd == NULL) return 0;

    if(strcmp(cmd, "who") == 0)
    {
        int i;
        char out[BUF_SIZE];
        for(i = 0; i < FD_SIZE; i++)
        {
            if(!client[i].occupied) continue;

            if((client + i) == sender)
                sprintf(out, "[Server] %-12s %s/%d ->me\n", sender->name, sender->ip, sender->port);
            else
                sprintf(out, "[Server] %-12s %s/%d\n", client[i].name, client[i].ip, client[i].port);

            strcat(sender_msg, out);
        }

        return 0;
    }
    else if(strcmp(cmd, "name") == 0)
    {
        char *name = strtok(NULL, "\a\t\r\n");

        if(name == NULL)
        {
            snprintf(sender_msg, BUF_SIZE, "[Server] ERROR: Error command.\n");
            return -1;
        }

        if(strcmp(name, "anonymous") == 0)
        {
            snprintf(sender_msg, BUF_SIZE, "[Server] ERROR: Username cannot be anonymous.\n");
            return 0;
        }
        else if(!validUsername(name))
        {
            snprintf(sender_msg, BUF_SIZE, "[Server] ERROR: Username can only consists of 2~12 English letters.\n");
            return 0;
        }
        else
        {
            int i;
            for(i = 0; i < FD_SIZE; i++)
            {
                if(!client[i].occupied) continue;

                if(strcmp(name, client[i].name) == 0)
                {
                    snprintf(sender_msg, BUF_SIZE, "[Server] ERROR: %s has been used by others.\n", name);
                    return 0;
                }
            }
        }

        //change sender's username
        snprintf(broadcast, BUF_SIZE, "[Server] %s is now known as %s.\n", sender->name, name);
        strcpy(sender->name, name);
        snprintf(sender_msg, BUF_SIZE, "[Server] You're now known as %s.\n", sender->name);

        return 0;
    }
    else if(strcmp(cmd, "tell") == 0)
    {
        char *client_name = strtok(NULL, TOK_DELIMETER);
        char *send_msg = strtok(NULL, "\t\a\r\n");
        if(client_name == NULL || send_msg == NULL)
        {
            snprintf(sender_msg, BUF_SIZE, "[Server] ERROR: Error command.\n");
            return -1;
        }

        while(*send_msg == ' ') send_msg++;

        if(strcmp(sender->name, "anonymous") == 0)
        {
            snprintf(sender_msg, BUF_SIZE, "[Server] ERROR: You are anonymous.\n");
            return 0;
        }
        else if(strcmp(client_name, "anonymous") == 0)
        {
            snprintf(sender_msg, BUF_SIZE, "[Server] ERROR: The client to which you sent is anonymous.\n");
            return 0;
        }
        else
        {
            int i;
            for(i = 0; i <FD_SIZE; i++)
            {
                if(!client[i].occupied) continue;

                if(strcmp(client[i].name, client_name) == 0)
                {
                    *recevier = client[i].fd;
                    snprintf(sender_msg, BUF_SIZE, "[Server] SUCCESS: Your message has been sent.\n");
                    snprintf(recevier_msg, BUF_SIZE, "[Server] %s tell you %s\n", sender->name, send_msg);
                    return 0;
                }
            }
            //the recevier does not exist, send error msg to sender
            snprintf(sender_msg, BUF_SIZE, "[Server] ERROR: The receiver doesn't exist.\n");
        }

        return 0;
    }
    else if(strcmp(cmd, "yell") == 0)
    {
        char *send_msg = strtok(NULL, "\a\r\n");
        if(send_msg == NULL)
        {
            snprintf(sender_msg, BUF_SIZE, "[Server] ERROR: Error command.\n");
            return -1;
        }

        while(*send_msg == ' ') send_msg++;

        snprintf(broadcast, BUF_SIZE, "[Server] %s yell %s\n", sender->name, send_msg);
        *b = -1;
        return 0;
    }
    else if(strcmp(cmd, "exit") == 0)
    {
        return 1;
    }
    else
    {
        snprintf(sender_msg, BUF_SIZE, "[Server] ERROR: Error command.\n");
        return -1;
    }
}

int Broadcast(struct Client client[], int sender, char *msg)
{
    int i;
    for(int i = 0; i < FD_SIZE; i++)
    {
        if(i != sender && client[i].occupied)
            Writen(client[i].fd, msg, strlen(msg));
    }
}

#endif