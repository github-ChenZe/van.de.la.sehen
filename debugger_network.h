#ifndef DEBUGGER_NETWORK_H
#define DEBUGGER_NETWORK_H

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <stdarg.h>

typedef struct sockaddr SA;

int open_clientfd(char *hostname, int port)
{
    int clientfd;
    struct hostent* hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    if ((hp = gethostbyname(hostname)) == NULL)
        return -2;
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char*) hp->h_addr_list[0], (char*)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    if (connect(clientfd, (SA*) &serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    return clientfd;
}

int clientfd, port = 14857;
char host[] = "localhost";
char debug_buf[1024];

int init_debugger()
{
    clientfd = open_clientfd(host, port);
}

int send_without_wait()
{
    write(clientfd, debug_buf, strlen(debug_buf));
}

int send_and_wait()
{
    write(clientfd, debug_buf, strlen(debug_buf));
    read(clientfd, debug_buf, 1024);
    // fputs(debug_buf, stdout);
}

int close_debugger()
{
    close(clientfd);
}

#endif