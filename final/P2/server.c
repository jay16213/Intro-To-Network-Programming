#include "headers.h"

int ndone;
int flags[1024];

pthread_mutex_t ndone_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ndone_cond = PTHREAD_COND_INITIALIZER;

struct Arguments {
    int id;
    int sockfd;
    struct sockaddr_in cli_addr;
    socklen_t clilen;
};

void do_sleep(void *params)
{
    int id = ((struct Arguments *) params)->id;
    int sockfd = ((struct Arguments *) params)->sockfd;
    struct sockaddr_in cli_addr = ((struct Arguments *) params)->cli_addr;
    socklen_t clilen = ((struct Arguments *) params)->clilen;

    int sleep_tv = rand() % 20 + 1;
    printf("%d sleep %d sec\n", id, sleep_tv);

    sleep(sleep_tv);
    char sendbuf[1024];
    sprintf(sendbuf, "res\n");

    if(sendto(sockfd, sendbuf, strlen(sendbuf) + 1, 0, (struct sockaddr *) &cli_addr, clilen) < 0)
    {
        perror("sendto error");
    }
      
    pthread_mutex_lock(&ndone_mutex);
    ndone++;
    flags[id] = 1;
    pthread_cond_signal(&ndone_cond);
    pthread_mutex_unlock(&ndone_mutex);

    free(params);
    pthread_exit(&sleep_tv);
}

void sig_alarm(int signo)
{
    pthread_cond_signal(&ndone_cond);
    return;
}

int main(int argc, char **argv)
{
    signal(SIGALRM, sig_alarm);
    siginterrupt(SIGALRM, 1);
    ndone = 0;

    pthread_t tid[1024];
    int used[1024];
    memset(used, 0, 1024);
    memset(flags, 0, 1024);

    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket error");
        exit(-1);
    }

    int flag = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);

    srand(time(NULL));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(34567);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind error");
        exit(-1);
    }

    int cnt = 0;
    while(1)
    {
        ssize_t len;
        char buf[1024];
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);

        if((len = recvfrom(sockfd, buf, 1024, 0, (struct sockaddr *) &cli_addr, &clilen)) < 0)
        {
            if(errno == EWOULDBLOCK) goto WAIT;

            perror("recvfrom error");
            exit(-1);
        }

        while(used[cnt]) cnt = (cnt + 1) % 1024;
        used[cnt] = 1;

        struct Arguments *args = (struct Arguments *) malloc(sizeof(struct Arguments));
        args->id = cnt;
        args->sockfd = sockfd;
        args->cli_addr = cli_addr;
        args->clilen = clilen;

        pthread_create(&tid[cnt], NULL, (void *) &do_sleep, (void *) args);

        WAIT:
            pthread_mutex_lock(&ndone_mutex);
            if(ndone == 0)
            {
                alarm(1);
                pthread_cond_wait(&ndone_cond, &ndone_mutex);     
                alarm(0);
            }

            if(ndone > 0)
            {
                int i;
                for(i = 0; i < 1024; i++)
                {
                    if(flags[i] > 0)
                    {
                        int *status;
                        pthread_join(tid[i], (void **) &status);
                        printf("thread(%d) return: %d\n", i, *status);
                        ndone--;
                        used[i] = 0;
                        flags[i] = 0;
                    }
                }
            }

            pthread_mutex_unlock(&ndone_mutex);
    }


    return 0;
}