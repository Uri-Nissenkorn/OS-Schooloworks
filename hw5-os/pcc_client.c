#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define BUFLEN 1024

int main(int argc, char const *argv[])
{
    struct sockaddr_in addr = {};
    int sock, b, i;
    uint64_t N = 0, netN;
    char *bufN;
    char *buf;
    char fileName[FILENAME_MAX] = {};

    if (argc != 4)
    {
        fprintf(stderr, "Invalid Input\n");
        exit(1);
    }


    // read ip
    memset(&addr, 0, sizeof(addr));

    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid IP Input\n");
        exit(1);
    }


    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));

    strcpy(fileName, argv[3]);


    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        fprintf(stderr, "Unable to Create Socket\n");
        exit(1);
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) >=0 )
    {
        FILE *fp = fopen(fileName, "rb");
        if (fp == NULL)
        {
            fprintf(stderr, "Unable to Open File\n");
            exit(1);
        }

        // calc N
        while (fgetc(fp) != EOF)
        {
            N++;
        }
        fseek(fp, 0, SEEK_SET);

        // send N
        netN = htonl(N);
        bufN = (char *)&netN;
        i = 0;
        b = 1;

        while (b > 0)
        {
            b = write(sock, bufN + i, 8 - i);
            i += b;
        }
        if (b < 0)
        {
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
            {
                fprintf(stderr, "TCP Error: %s\n", strerror(errno));
                close(sock);
                exit(1);
            }
            else
            {
                fprintf(stderr, "Unexpexted Error: %s\n", strerror(errno));
                exit(1);
            }
        }
        if (i != 8)
        {
            fprintf(stderr, "Connection Error: %s\n", strerror(errno));
            close(sock);
            exit(1);
        }

        // send file
        i = 0;
        b = 1;
        buf = malloc(N);
        while (b > 0)
        {
            b = fread(buf, 1, BUFLEN, fp);
            write(sock, buf + i, N-i);
            i+=b;
        }
        fclose(fp);
        free(buf);

        if (b < 0)
        {
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
            {
                fprintf(stderr, "TCP Error: %s\n", strerror(errno));
                close(sock);
                exit(1);
            }
            else
            {
                // wrong error
                fprintf(stderr, "Unexpexted Error: %s\n", strerror(errno));
                close(sock);
                exit(1);
            }
        }
        if (i != N)
        {
            fprintf(stderr, "Connection Error: %s\n", strerror(errno));
            close(sock);
            exit(1);
        }

        // recive R
        bufN = (char *)malloc(sizeof(uint64_t));
        i = 0;
        while ((b = read(sock, bufN + i, 8 - i)) > 0)
        {
            i += b;
        }
        if (b < 0)
        {
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
            {
                fprintf(stderr, "TCP Error: %s\n", strerror(errno));
                close(sock);
                free(bufN);
                exit(1);
            }
            else
            {
                // wrong error
                fprintf(stderr, "Unexpexted Error: %s\n", strerror(errno));
                close(sock);
                free(bufN);
                exit(1);
            }
        }
        if (i != 8)
        {
            fprintf(stderr, "Connection Error: %s\n", strerror(errno));
            close(sock);
            free(bufN);
            exit(1);
        }
        N = ntohl(bufN);
        close(sock);
    }
    else
    {
         fprintf(stderr, "Connection Error: %s\n", strerror(errno));
        exit(1);
    }

    return 0;
}
