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
    struct timeval timeout;

    init_log();
    timeout.tv_sec  = 0.01;
    timeout.tv_usec = 0;


    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);

    listenfd = Open_listenfd(port);
    init_pool(listenfd, &pool);
    while (1) {
        pool.ready_set = pool.read_set; // only ready_set is being updated by Select(side effect of Select), read_set is the ones to be watched
        //TODO: enable Select to be synced with Rio read with buffer
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, &timeout);

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
    FD_ZERO(&p->reading_left_set);
    FD_ZERO(&p->read_set);
    FD_ZERO(&p->pipeline_ready_set);
    FD_SET(listenfd, &p->read_set);
}


void add_client(int connfd, pool *p) {
    int i;
    p->nready--;
    fcntl(connfd, F_SETFL, O_NONBLOCK);

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
    char usrbuf[RIO_BUFSIZE];

    for (i = 0; i <= p->maxi && (p->nready > 0 || p->pipeline_nready > 0 || p->reading_left_nready > 0); i++) {
        memset(usrbuf, 0, RIO_BUFSIZE);
        connfd = p->clientfd[i];
        rio=&(p->clientrio[i]);

        if (connfd > 0 && (FD_ISSET(connfd, &p->ready_set) || FD_ISSET(connfd, &p->pipeline_ready_set) || FD_ISSET(connfd, &p->reading_left_set))) {
            if (FD_ISSET(connfd, &p->ready_set)) {
                p->nready--;
            } 
            if (FD_ISSET(connfd, &p->pipeline_ready_set)) {
                log_info("pipeline handler is invoked\n");
                p->pipeline_nready--;
                FD_CLR(connfd, &p->pipeline_ready_set);
            } 
            if ((n = Rio_readn(rio, usrbuf, MAXLINE)) > 0) {
                Request *request = NULL;
                //if (p->clientReadingLeft[i] == 0 ) {
                        request = parse_header(usrbuf, n, connfd);
                        if (request == NULL) {
                            Rio_writen(connfd, BAD_REQUEST, strlen(BAD_REQUEST));
                            log_error("request: %s\n response: %s\n", usrbuf, BAD_REQUEST);
                        } else {
                            move_rio_bufptr(rio, request->header_offset - n);
                            int m;
                            if ((m = Rio_readn(rio, request->message_body, request->content_length)) == request->content_length) {
                                char* response_buf = (char*) malloc(sizeof(char) * (request->content_length + request->header_offset));
                                strncpy(response_buf, usrbuf, request->header_offset);
                                log_info("header offset %d\n", request->header_offset);
                                log_info("usrbuf: %s with content_length %d\n", response_buf, request->content_length);
                                strncpy(response_buf+request->header_offset, request->message_body, request->content_length);
                                log_info("request: %s\n", response_buf);
                                Rio_writen(connfd, response_buf, request->content_length + request->header_offset);
                            } else {
                                log_error("request: %s\n response: %s\n", usrbuf, BAD_REQUEST);
                                Rio_writen(connfd, BAD_REQUEST, strlen(BAD_REQUEST));
                            }

                            if (n-request->header_offset > request->content_length) {// When there are remaining pipeline request
                                //TODO: pipeline    
                                log_error("doesn't support pipeline");
                            }
                            /*Rio_writen(connfd, usrbuf, n-request->unhandled_buffer_size); 
                            //log_info("request: %s\n ", usrbuf);

                            if (request->content_length > 0 && request->message_body_size > 0 && 
                                    request->message_body_size < request->content_length) {
                                p->clientReadingLeft[i] = request->content_length - request->message_body_size;
                                p->reading_left_nready += 1;
                                FD_SET(connfd, &p->reading_left_set);
                            } else if (request->unhandled_buffer_size > 0) {
                                log_info("unhandled_buffer_size is %d\n", request->unhandled_buffer_size);
                                move_rio_bufptr(rio, -request->unhandled_buffer_size);
                                p->pipeline_nready += 1;
                                FD_SET(connfd, &p->pipeline_ready_set); 
                            }*/
                        }
                        //TODO: shouldn't call twice for unfinished request, should in one loop. 
                //} else {
                //    Rio_writen(connfd, usrbuf, MIN(n, p->clientReadingLeft[i])); 
                //    if (n >= p->clientReadingLeft[i]) {
                //        move_rio_bufptr(rio, p->clientReadingLeft[i]-n); 
                //        p->pipeline_nready += 1;
                //        FD_SET(connfd, &p->pipeline_ready_set); 

                //        log_info("reading left is invoked\n");
                //        p->reading_left_nready--;
                //        FD_CLR(connfd, &p->reading_left_set);
                //    }
                //    p->clientReadingLeft[i] -= MIN(n, p->clientReadingLeft[i]);
                //} 
            } else if (n == 0) {
                close_connection(p, connfd, i);
            } 
        } 
    }
}


