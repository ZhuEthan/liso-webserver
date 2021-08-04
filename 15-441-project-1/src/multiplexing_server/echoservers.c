#include "csapp.h"
#include "parse.h"
#include "util/log.h"

int byte_cnt = 0;
char* BAD_REQUEST = "HTTP/1.1 400 Bad Request\r\n\r\n";

void close_connection(pool* p, int connfd, int fdIndex) {
    FD_CLR(connfd, &p->read_set);// clear read_set instead of ready_set, ready_set is just the one to be read
    p->clientfd[fdIndex] = -1;
    p->clientReadingLeft[fdIndex] = 0;
    Close(connfd);
}

int main(int argc, char **argv) {
    int listenfd, connfd, port;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    static pool pool;
    init_log();

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);

    listenfd = Open_listenfd(port);
    init_pool(listenfd, &pool);
    while (1) {
        pool.ready_set = pool.read_set; // only ready_set is being updated by Select(side effect of Select), read_set is the ones to be watched
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

        printf("nready is %x\n", pool.nready);
        if (FD_ISSET(listenfd, &pool.ready_set)) {
            fprintf(stdout, "accept listenfd %d\n", listenfd);
            connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
            add_client(connfd, &pool);
        }

        check_client(&pool);
    }

    close_log();
}


void init_pool(int listenfd, pool *p) {
    int i;
    p->maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++) {
        p->clientfd[i] = -1;
        p->clientReadingLeft[i] = 0;
    }

    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}


void add_client(int connfd, pool *p) {
    int i;
    p->nready--;
    for (int i = 0; i < FD_SETSIZE; i++) {
        if (p->clientfd[i] < 0) {
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);

            FD_SET(connfd, &p->read_set);

            if (connfd > p->maxfd) {
                p->maxfd = connfd;
            }
            if (i > p->maxi) {
                p->maxi = i;
            }

            break;
        }
        if (i == FD_SETSIZE) {
            app_error("add_client error: Too many clients");
        }
    }
}

void check_client(pool *p) {
    int i, connfd, n;
    rio_t* rio;

    fprintf(stdout, "p->nready %d\n", p->nready);
    for (i = 0; i <= p->maxi && p->nready > 0; i++) {
        connfd = p->clientfd[i];
        rio=&(p->clientrio[i]);

        if (connfd > 0 && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            if ((n = Rio_read(rio, rio->rio_buf, MAXLINE)) != 0) {
                fprintf(stdout, "reading %d chars %s", n, rio->rio_buf);
                Request *request = NULL;
                if (p->clientReadingLeft[i] == 0) {
                    request = parse(rio->rio_buf, n, connfd);
                    //pipelines: reading message_length and reparse the remaining one, change the IO to be non-blocking
                    if (request == NULL) {
                        Rio_writen(connfd, BAD_REQUEST, strlen(BAD_REQUEST));
                        log_error("request: %s\n response: %s\n", rio->rio_buf, BAD_REQUEST);
                    } else {
                        if (request->content_length != 0 && request->message_body != NULL && 
                                    strlen(request->message_body) < request->content_length) {
                            p->clientReadingLeft[i] = request->content_length - strlen(request->message_body);
                        }
                        Rio_writen(connfd, rio->rio_buf, n); 
                        log_info("request: %s\n response: %s\n", rio->rio_buf, rio->rio_buf);
                    }
                } else {
                    Rio_writen(connfd, rio->rio_buf, n); 
                    p->clientReadingLeft[i] -= n;
                    log_info("request: %s\n response: %s\n", rio->rio_buf, rio->rio_buf);
                } 
            } else {
                close_connection(p, connfd, i);
            }
        } 
    }
}


