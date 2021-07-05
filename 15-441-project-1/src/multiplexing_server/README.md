## Usage: 
> gcc -g echoservers.c csapp.c -o server
> ./server 9999

## TODO: 
* Current implementation can only handle message body with \n, however, the HTTP [RFC](https://www.ietf.org/rfc/rfc2616.txt) allows message body ending without with \n. hint: readn based on Content-Length instead of readline to be used. 
* How to switch among multiple connections? Now it doesn't consider a lot. 