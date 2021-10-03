## Usage: 
> make select_server

> ./select_server 9999

> ab -n 10 -c 10 http://127.0.0.1:9999/

> python cp1_checker.py 127.0.0.1 9999 100 10 10


## Appendix
* Client/Server call sequence is as [here](https://www.cs.dartmouth.edu/~campbell/cs50/socketprogramming.html). Notice that server must have a "read" before close() after client close related socket. 
