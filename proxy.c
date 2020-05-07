/*
* proxy.c: an implementation of a sequential web proxy
*/

#include <stdio.h>
#include "csapp.h"

void do_proxy(int connfd);
void parse_url(char *url, char *hostname, char *port, char *uri);
void read_hdrs(rio_t *rp, char *hdrs);
void proxy(int clientfd, char *method, char *uri, char *hdrs, int connfd);
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char *argv[])
{
    int listenfd, connfd;
    SA clientaddr;
    socklen_t clientlen;
    char hostname[MAXLINE], port[MAXLINE];

    if(argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, &clientaddr, &clientlen);
        Getnameinfo(&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        do_proxy(connfd);
    }
    return 0;
}

void do_proxy(int connfd)
{
    rio_t rio;
    char buf[MAXLINE];
    char method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE], uri[MAXLINE];
    char hdrs[MAXLINE];
    Rio_readinitb(&rio, connfd);

    /* request line process */
    Rio_readlineb(&rio, (void *)buf, MAXLINE);
    printf("From src: %s\n", buf);
    sscanf(buf, "%s %s %s", method, url, version);

    /* url->hostname, port, uri */
    parse_url(url, hostname, port, uri);

    /* request headers process */
    read_hdrs(&rio, hdrs);

    /* forward request line and headers to server and forward response to client */
    int clientfd = Open_clientfd(hostname, port);
    proxy(clientfd, method, uri, hdrs, connfd);
}

void parse_url(char *url, char *hostname, char *port, char *uri)
{
    char *p;
    strcpy(uri, strchr(url+7, '/'));
    p = strchr(url+7, '/');
    *p = '\0';
    strcpy(hostname, strtok(url+7, ":"));
    strcpy(port, strtok(NULL, ":"));
}

void read_hdrs(rio_t *rp, char *hdrs)
{
    char buf[MAXLINE];
    Rio_readlineb(rp, (void *)buf, MAXLINE);
    while(strcmp(buf, "\r\n"))
    {
        if(strstr(buf, "User-Agent") != NULL)
        {
            sprintf(hdrs, "%s%s", hdrs, user_agent_hdr);
        } else if(strstr(buf, "Connection") != NULL)
        {
            // do nothing
        } else
        {
            sprintf(hdrs, "%s%s", hdrs, buf);
        }
        Rio_readlineb(rp, buf, MAXLINE);
    }
    sprintf(hdrs, "%s%s", hdrs, "Proxy-Connection: close\r\n");
    sprintf(hdrs, "%s%s", hdrs, "Connection: close\r\n\r\n");
}

void proxy(int clientfd, char *method, char *uri, char *hdrs, int connfd)
{
    rio_t rio;
    Rio_readinitb(&rio, clientfd);
    int readn;
    char buf[MAXLINE];
    sprintf(buf, "%s %s HTTP/1.0\r\n%s", method, uri, hdrs);
    Rio_writen(clientfd, (void *)buf, strlen(buf));
    while((readn = Rio_readlineb(&rio, (void *)buf, MAXLINE)) != 0)
    {
        Rio_writen(connfd, (void *)buf, readn);
    }
}