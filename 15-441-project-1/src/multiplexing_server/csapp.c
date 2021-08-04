#include "csapp.h"

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
	    unix_error("Rio_readlineb error");
    return rc;
} 

ssize_t Rio_readn(rio_t* rio, void *usrbuf, size_t n) {
    ssize_t rc;

    if ((rc = rio_readn(rio, usrbuf, n)) < 0) {
        unix_error("Rio_readn error");
    }

    return rc;
}

ssize_t Rio_read(rio_t *rp, void *usrbuf, size_t n) {
    ssize_t rc;

    if ((rc = rio_read(rp, usrbuf, n)) < 0) {
        unix_error("Rio_read error");
    }

    return rc;
}


/* 
 * rio_readlineb - Robustly read a text line (buffered)
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) { 
        if ((rc = rio_read(rp, &c, 1)) == 1) {
	        *bufp++ = c;
	        if (c == '\n') {
                n++;
     		    break;
            }
	    } else if (rc == 0) {
	        if (n == 1)
		        return 0; /* EOF, no data read */
	        else
		        break;    /* EOF, some data was read */
	    } else
	        return -1;	  /* Error */
    }
    *bufp = 0;
    return n-1;
}

/* 
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* Refill if buf is empty */
	    rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, 
			   sizeof(rp->rio_buf));
	    if (rp->rio_cnt < 0) {
	        if (errno != EINTR) { /* Interrupted by sig handler return */
		        return -1;
            }
        } else if (rp->rio_cnt == 0)  /* EOF */
	        return 0;
	    else 
	        rp->rio_bufptr = rp->rio_buf; /* Reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;          
    if (rp->rio_cnt < n)   
	    cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}


ssize_t rio_readn(rio_t* rio, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char* bufp = usrbuf;

    while (nleft > 0) {
        if ((nread = rio_read(rio, bufp, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;
            } else {
                return -1;
            }
        } else if (nread == 0) {
            break;
        }
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);
}

void Rio_writen(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n)
	    unix_error("Rio_writen error");
}

ssize_t rio_writen(int fd, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    char* bufp = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR) {
                nwritten = 0;
            } else {
                return -1;
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }

    return n;
}

void Rio_readinitb(rio_t *rp, int fd) {
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

int Select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, 
    struct timeval *timeout) {
    return select(n, readfds, writefds, exceptfds, timeout);
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    return accept(s, addr, addrlen);
}

int Open_listenfd(int port) {
    printf("open listen fd %d\n", port);
    return open_listenfd(port);
}

int open_listenfd(int port) {
    int listenfd;
    struct sockaddr_in addr, cli_addr;

    if ((listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    int flags = fcntl(listenfd, F_GETFL); //"could not get file flags";
    fcntl(listenfd, F_SETFL, flags | O_NONBLOCK); //"could not set file flags";


    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd, (struct sockaddr *) &addr, sizeof(addr)))
    {
        Close(listenfd);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }

    if (listen(listenfd, 100))
    {
        Close(listenfd);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    return listenfd;

}


void Close(int fd) 
{
    printf("close fd %d\n", fd);
    int rc;

    if ((rc = close(fd)) < 0)
	    unix_error("Close error");
}

/* $begin unixerror */
void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

void app_error(char *msg) /* Application error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}
