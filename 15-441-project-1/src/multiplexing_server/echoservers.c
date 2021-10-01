#include "csapp.h"
#include "parse.h"
#include "util/log.h"
#include "cgi_util.h"

int byte_cnt = 0;
char* BAD_REQUEST = "HTTP/1.1 400 Bad Request\r\n\r\n";

void close_connection(pool* p, int connfd, int fdIndex) {
    FD_CLR(connfd, &p->read_set);// clear read_set instead of ready_set, ready_set is just the one to be read
    p->clientfd[fdIndex] = -1;
    Close(connfd);
}

int main(int argc, char **argv) {
    int listenfd, connfd, port;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    static pool pool;
    struct timeval timeout;

    init_log();
    timeout.tv_sec  = 10;//0.01;
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
        int nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, &timeout);

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
    }

    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_ZERO(&p->pipeline_ready_set);
    FD_SET(listenfd, &p->read_set);
}


void add_client(int connfd, pool *p) {
    int i;
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

char* reading_file(char* fileName) {
    char* buff = (char*) malloc(MAXLINE * sizeof(char));
    log_info("fileName is %s", fileName);
    FILE *fp = fopen(fileName, "r");
    if (!fp) {
        log_info("fopen failed %s", strerror(errno));
        return NULL;
    }
    fgets(buff, MAXLINE, fp);
    log_info("file content is %s", buff);
    fclose(fp);
    return buff;
}

char* get_last_modified_date(char* fileName) {
    struct stat buf;
    Stat(fileName, &buf);
    char* date = (char*) malloc(10 * sizeof(char));
    strftime(date, 10, "%d-%m-%y", gmtime(&(buf.st_ctime)));
    log_info("date: %s", date);
    return date;
}

char* extractRelativePath(char* path) {
    log_info("path is %s", path);
    char* pointerToDNSName = strstr(path, "127.0.0.1");
    if (pointerToDNSName == NULL) {
        if (path[0] == '/') {
            return path + 1;
        }
        return path;
    }
    return pointerToDNSName + 9;
}

void check_client(pool *p) {
    int i, connfd, n;
    rio_t* rio;
    char usrbuf[RIO_BUFSIZE];

    for (i = 0; i <= p->maxi; i++) {
        memset(usrbuf, 0, RIO_BUFSIZE);
        connfd = p->clientfd[i];
        rio=&(p->clientrio[i]);

        if (connfd > 0 && (FD_ISSET(connfd, &p->ready_set) || FD_ISSET(connfd, &p->pipeline_ready_set))) {
            if (FD_ISSET(connfd, &p->pipeline_ready_set)) {
                log_info("pipeline handler is invoked\n");
                FD_CLR(connfd, &p->pipeline_ready_set);
            } 
            if ((n = Rio_readn(rio, usrbuf, MAXLINE)) > 0) {
                Request *request = NULL;
                request = parse_header(usrbuf, n, connfd);
                if (request == NULL) {
                    Rio_writen(connfd, BAD_REQUEST, strlen(BAD_REQUEST));
                    log_error("request: %s\n response: %s\n", usrbuf, BAD_REQUEST);
                } else {
                    move_rio_bufptr(rio, request->header_offset - n);
                    int m;
                    if ((m = Rio_readn(rio, request->message_body, request->content_length)) == request->content_length) {
                        /*char* response_buf = (char*) malloc(sizeof(char) * (request->content_length + request->header_offset));
                        strncpy(response_buf, usrbuf, request->header_offset);
                        log_info("header offset %d\n", request->header_offset);
                        log_info("usrbuf: %s with content_length %d\n", response_buf, request->content_length);
                        strncpy(response_buf+request->header_offset, request->message_body, request->content_length);
                        log_info("request: %s\n", response_buf);
                        Rio_writen(connfd, response_buf, request->content_length + request->header_offset);*/
                        char* response = (char*) malloc(MAXLINE * sizeof(char));
                        if (strcmp("GET", request->http_method) == 0 || strcmp("HEAD", request->http_method) == 0) {
                            char* relativePath = extractRelativePath(request->http_uri);
                            char* path;
                            char* content;

                            log_info("relativepath is %s", relativePath);
                            fprintf(stdout, "%s", relativePath);
                            if (strncmp(relativePath, "cgi", 3) == 0) {
                                //TODO: call cgi_util.c
                                log_info("I am in cgi");
                                content = cgi_start(request->message_body, request->headers);
                                log_info("content %s", content);
                                sprintf(response, "%s %s %s\r\nContent-Length: %d\r\n\r\n%s",
                                        request->http_version, "400", "Reason-Phrase", strlen(content), content);
                                Rio_writen(connfd, response, strlen(response));
                            } else {
                                path = (char*) malloc(strlen(relativePath)+1);
                                sprintf(path, "cp2/static_site/%s", relativePath);
                                /** 
                                *read files using some FD. 
                                **/
                                content = reading_file(path);
                            
                                if (content == NULL) {
                                    sprintf(response, "%s %s %s\r\n\r\n", request->http_version, "404", "resource not found");
                                    Rio_writen(connfd, response, strlen(response));
                                } else {
                                    char* last_modified_date = get_last_modified_date(path);
                            
                                    char* status_code = (char*) malloc(4);
                                    strcpy(status_code, "200");

                                    if (strcmp("HEAD", request->http_method) == 0) {
                                        log_info("HEAD is triggered");
                                        sprintf(response, "%s %s %s\r\nContent-Length: %d\r\nLast-Modified: %s\r\n\r\n",
                                            request->http_version, status_code, "Reason-Phrase", strlen(content), last_modified_date);
                                    } else if (strcmp("GET", request->http_method) == 0) {
                                        log_info("HEAD is triggered");
                                        sprintf(response, "%s %s %s\r\nContent-Length: %d\r\nLast-Modified: %s\r\n\r\n%s",
                                            request->http_version, status_code, "Reason-Phrase", strlen(content), last_modified_date, content);
                                    }
                                    log_info("response length is %s with length %d", response,  strlen(response));
                                    Rio_writen(connfd, response, strlen(response));
                                }
                            }
                        } else {
                            sprintf(response, "%s %s %s\r\n\r\n", request->http_version, "500", "Reason-Phrase");
                            Rio_writen(connfd, response, strlen(response));
                        }
                    } else {
                        log_error("request: %s\n response: %s\n", usrbuf, BAD_REQUEST);
                        Rio_writen(connfd, BAD_REQUEST, strlen(BAD_REQUEST));
                    }

                    if (n-request->header_offset > request->content_length) {// When there are remaining pipeline request
                        log_info("pipeline support\n");
                        FD_SET(connfd, &p->pipeline_ready_set);
                    }
                  
                }
            } else if (n == 0) {
                close_connection(p, connfd, i);
            } 
        } 
    }
}


