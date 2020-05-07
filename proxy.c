/*
* proxy.c: an implementation of a sequential web proxy
*/

#include <stdio.h>
#include "csapp.h"

void do_proxy(int connfd);
void parse_url(char *url, char *hostname, char *port, char *uri);
int read_hdrs(rio_t *rp, char *hdrs);
int proxy(int clientfd, char *method, char *uri, char *hdrs, int connfd);
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
    if(Rio_readlineb(&rio, (void *)buf, MAXLINE) == -1)
    {
        unix_error("read request line error");
        return;
    }
    sscanf(buf, "%s %s %s", method, url, version);

    /* url->hostname, port, uri */
    parse_url(url, hostname, port, uri);

    /* request headers process */
    if(read_hdrs(&rio, hdrs) == -1)
    {
        unix_error("read hdrs error");
        close(connfd);
        return;
    }

    /* forward request line and headers to server and forward response to client */
    int clientfd = Open_clientfd(hostname, port);
    if(clientfd == -1)
    {
        unix_error("open clientfd error");
        close(connfd);
        return;
    }
    if(proxy(clientfd, method, uri, hdrs, connfd) == -1)
    {
        unix_error("proxy error");
        close(connfd);
        close(clientfd);
        return;
    }
    close(connfd);
    close(clientfd);
}

void parse_url(char *url, char *hostname, char *port, char *uri)
{
    char *p;
    strcpy(uri, strchr(url+7, '/'));
    p = strchr(url+7, '/');
    *p = '\0';
    strcpy(hostname, strtok(url+7, ":"));
    if((p = strtok(NULL, ":")) == NULL)
    {
        strcpy(port, "80");
    }else
    {
        strcpy(port, p);
    }
}

int read_hdrs(rio_t *rp, char *hdrs)
{
    char buf[MAXLINE];
    int readn;
    readn = Rio_readlineb(rp, (void *)buf, MAXLINE);
    if(readn == -1)
    {
        return -1;
    }
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
        readn = Rio_readlineb(rp, buf, MAXLINE);
        if(readn == -1)
        {
            return -1;
        }
    }
    sprintf(hdrs, "%s%s", hdrs, "Proxy-Connection: close\r\n");
    sprintf(hdrs, "%s%s", hdrs, "Connection: close\r\n\r\n");
    return 0;
}

int proxy(int clientfd, char *method, char *uri, char *hdrs, int connfd)
{
    rio_t rio;
    Rio_readinitb(&rio, clientfd);
    int readn;
    char buf[MAXLINE];
    sprintf(buf, "%s %s HTTP/1.0\r\n%s", method, uri, hdrs);
    if(Rio_writen(clientfd, (void *)buf, strlen(buf)) == -1)
    {
        return -1;
    }
    while((readn = Rio_readlineb(&rio, (void *)buf, MAXLINE)) != 0)
    {
        if(Rio_writen(connfd, (void *)buf, readn) == -1)
        {
            return -1;
        }
    }
    return 0;
}