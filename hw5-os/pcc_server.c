#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>

#define BUFLEN 1024

int confd = -1;
int sigint_flag = 0;

uint64_t pcc_total[127] = {0};

void close_server()
{
    for (int i = 32; i < 127; i++)
    {
        printf("char '%c' : %lu times\n", (i), pcc_total[i]);
    }
    exit(0);
}

void sigint_handler()
{
    if (confd < 0)
    {
        close_server();
    }
    else
    {
        sigint_flag = 1;
    }
}

int main(int argc, char const *argv[])
{
    uint64_t pcc_tmp[127] = {};
    int confd = 0, b, i, listenfd = -1, rt = 1;
    struct sockaddr_in addr;
    uint64_t N = 0, R=0;
    char *bufN;
    char *buf;

    if (argc != 2)
    {
        fprintf(stderr, "Invalid Input\n");
        exit(1);
    }

    // singit
    struct sigaction sigint;
    sigint.sa_handler = &sigint_handler;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sigint, 0) != 0)
    {
        fprintf(stderr, "Signal handle registration failed. Error: %s\n", strerror(errno));
        exit(1);
    }

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Could Not Create Socket\n");
        exit(1);
    }
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &rt, sizeof(int)) < 0)
    {
        fprintf(stderr, "Setsockopt Error\n");
        exit(1);
    }

    // read ip
    memset(&addr, 0, sizeof(addr));
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid IP\n");
        exit(1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));

    if (0 != bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)))
    {
        fprintf(stderr, "Bind Failed, Error: %s \n", strerror(errno));
        exit(1);
    }

    if (0 != listen(listenfd, 10))
    {
        fprintf(stderr, "Error : Listen Failed. %s \n", strerror(errno));
        exit(1);
    }

    while (1)
    {
        if (sigint_flag)
        {
            close_server();
        }

        // accept
        confd = accept(listenfd, NULL, NULL);
        if (confd == -1)
        {
            fprintf(stderr, "Accept Failed, Error: %s\n", strerror(errno));
            continue;
        }

        bufN = (char *)malloc(sizeof(uint64_t));
        i = 0;
        while ((b = read(confd, bufN + i, 8 - i)) > 0)
        {
            i += b;
        }
        if (b < 0)
        {
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
            {
                fprintf(stderr, "TCP Error: %s\n", strerror(errno));
                close(confd);
                confd = -1;
                continue;
            }
            else
            {
                // wrong error
                fprintf(stderr, "Unexpexted Error: %s\n", strerror(errno));
                free(bufN);
                exit(1);
            }
        }
        if (i != 8)
        {
            fprintf(stderr, "Connection Error: %s\n", strerror(errno));
            close(confd);
            confd = -1;
            continue;
        }

        N = ntohl(bufN);

        buf = (char *)malloc(sizeof(char) * N);
        i = 0;
        while ((b = read(confd, buf + i, N - i)) > 0)
        {
            i += b;
        }
        if (b < 0)
        {
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
            {
                fprintf(stderr, "TCP Error: %s\n", strerror(errno));
                close(confd);
                confd = -1;
                continue;
            }
            else
            {
                // wrong error
                fprintf(stderr, "Unexpexted Error: %s\n", strerror(errno));
                free(bufN);
                exit(1);
            }
        }
        if (i != N)
        {
            fprintf(stderr, "Connection Error: %s\n", strerror(errno));
            close(confd);
            confd = -1;
            continue;
        }

        // readable chars
        R=0;
        for(i=32;i<127;i++) {
            pcc_tmp[i]=0;
        }
        
        for (i = 0; i < N; i++)
        {
            if (32 <= *(buf+i) && *(buf+i) <= 126){
                R++;
                pcc_tmp[(int)(*(buf+i))]++;
            }
            
        }

        // return R;
        R = htonl(R);
        i=0;
        free(bufN);
        bufN = (char *)&R;
        while ((b = write(confd, bufN + i, 8 - i)) > 0)
        {
            i += b;
        }
        if (b < 0)
        {
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
            {
                fprintf(stderr, "TCP Error: %s\n", strerror(errno));
                close(confd);
                confd = -1;
                continue;
            }
            else
            {
                // wrong error
                fprintf(stderr, "Unexpexted Error: %s\n", strerror(errno));
                free(bufN);
                exit(1);
            }
        }
        if (i != 8)
        {
            fprintf(stderr, "Connection Error: %s\n", strerror(errno));
            close(confd);
            confd = -1;
            continue;
        }

        for(i=32;i<127;i++) {
            pcc_total[i] += pcc_tmp[i];
        }

        close(confd);
        confd = -1;
    }
}
