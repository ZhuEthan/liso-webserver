## Usage: 
> make select_server

> ./select_server 9999

> ab -n 10 -c 10 http://127.0.0.1:9999/

> python cp1_checker.py 127.0.0.1 9999 100 10 10

## TODO: 
* Current implementation can only handle message body with \n, however, the HTTP [RFC](https://www.ietf.org/rfc/rfc2616.txt) allows message body ending without \n. hint: readn based on Content-Length instead of readline to be used. 

> The presence of a message-body in a request is signaled by the
inclusion of a Content-Length or Transfer-Encoding header field in
the request's message-headers. A message-body MUST NOT be included in
a request if the specification of the request method (section 5.1.1)
does not allow sending an entity-body in requests. A server SHOULD
read and forward a message-body on any request; if the request method
does not include defined semantics for an entity-body, then the
message-body SHOULD be ignored when handling the request.

> Yes, you may reject any header line larger than 8192 bytes. Note that this is different than a “Content- Length” of greater than 8192. Additionally, if you do reject a request, you must find and parse the next request properly for pipelining purposes

> HTTP requests and responses can be pipelined on a connection.
Pipelining allows a client to make multiple requests without
waiting for each response, allowing a single TCP connection to
be used much more efficiently, with much lower elapsed time.


* How to switch among multiple connections? Now it doesn't consider a lot. 

## Appendix
* Client/Server call sequence is as [here](https://www.cs.dartmouth.edu/~campbell/cs50/socketprogramming.html). Notice that server must have a "read" before close() after client close related socket. 