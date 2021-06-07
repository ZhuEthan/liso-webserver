#include <csapp.h>

typedef struct { // Represents a pool of connected descriptors
    int maxfd; // Largest descriptor in read_set
    fd_set read_set; // Set of all active descriptors
    fd_set ready_set; // Subset of descriptors ready for reading
    int nready; // Number of ready descriptors from select
    int maxi; // Highwater index into client array
    int clientfd[FD_SETSIZE]; // Set of active descriptors
    rio_t clientrio[FD_SETSIZE]; // Set of active read buffers
} pool;

int byte_cnt = 0;

int main(int argc, char **argv) {
    int listenfd, connfd, port;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    static pool pool;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exist(0);
    }
    port = atoi(argv[1]);

    listenfd = Open_listenfd(port);
    init_pool(listenfd, &pool);
    while (1) {
        pool.ready_set = pool.read_set; // only ready_set is being updated by Select(side effect of Select), read_set is the ones to be watched
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &pool.ready_set)) {
            connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
            add_client(connfd, &pool);
        }

        check_client(&pool);
    }
}


void init_pool(int listenfd, pool *p) {
    int i;
    p->maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++) {
        p->clientfd[i] = -1;
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
    char buf[MAXLINE];
    rio_t rio;

    for (i = 0; i < p->maxi && p->nready > 0; i++) {
        connfd = p->clientfd[i];
        rio=p->clientrio[i];

        if (connfd > 0 && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            if (n == Rio_readlineb(&rio, buf, MAXLINE) != 0) {
                byte_cnt += n;
                printf("Server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
                Rio_writen(connfd, buf, n);
            } else {
                Close(connfd);
                FD_CLR(connfd, &p->read_set); // clear read_set instead of ready_set, ready_set is just the one to be read
                p->clientfd[i] = -1;
            }
        } 
    }
}