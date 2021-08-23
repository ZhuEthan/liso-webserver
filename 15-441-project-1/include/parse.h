#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SUCCESS 0

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

//Header field
typedef struct
{
	char header_name[4096];
	char header_value[4096];
} Request_header;

//HTTP Request Header
typedef struct
{
	char http_version[50];
	char http_method[50];
	char http_uri[4096];
	Request_header *headers;
	int header_count;
	int header_offset;
	int content_length;

	char* message_body;
	int message_body_size;

	char unhandled_buffer[8192];
	int unhandled_buffer_size;
} Request;

Request* parse_header(char *buffer, int size,int socketFd);

// functions decalred in parser.y
int yyparse();
typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_create_buffer (FILE *file,int size);
void yy_switch_to_buffer  (YY_BUFFER_STATE  new_buffer );
void set_parsing_options(char *buf, size_t i, Request *request);
