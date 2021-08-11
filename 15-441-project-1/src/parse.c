#include "parse.h"

#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))
/**
* Given a char buffer returns the parsed request headers
*/
Request * parse(char *buffer, int size, int socketFd) {
  //Differant states in the state machine
	enum {
		STATE_START = 0, STATE_CR, STATE_CRLF, STATE_CRLFCR, STATE_CRLFCRLF
	};

	int i = 0, state;
	size_t header_offset = 0;
	char ch;
	char buf[8192];
	memset(buf, 0, 8192);

	int crlf_num = 0;

	state = STATE_START;
	while (state != STATE_CRLFCRLF) {
		char expected = 0;

		if (i == size)
			break;

		ch = buffer[i++];
		buf[header_offset++] = ch;

		switch (state) {
		case STATE_START:
		case STATE_CRLF:
			expected = '\r';
			break;
		case STATE_CR:
		case STATE_CRLFCR:
			crlf_num += 1;
			expected = '\n';
			break;
		default:
			state = STATE_START;
			continue;
		}

		if (ch == expected)
			state++;
		else
			state = STATE_START;

	}

    //Valid End State
	if (state == STATE_CRLFCRLF) {
		Request *request = (Request *) malloc(sizeof(Request));
        request->header_count=0;
        //TODO You will need to handle resizing this in parser.y
		printf("request->header number is %d\n", crlf_num-2);
        request->headers = (Request_header *) malloc(sizeof(Request_header)*MAX(0, crlf_num-2));
		set_parsing_options(buf, i, request);

		if (yyparse() == SUCCESS) {
			if (request->content_length > 0) {
				request->message_body_size = MIN(request->content_length, size-header_offset);
				memcpy(request->message_body, buffer+header_offset, request->message_body_size);

				request->unhandled_buffer_size = MAX(size-header_offset-(request->message_body_size), 0);
				memcpy(request->unhandled_buffer, buffer+header_offset+request->message_body_size, request->unhandled_buffer_size);
				//memcpy(request->unhandled_buffer, buffer+offset+request->content_length, size-offset-(request->content_length));
				printf("request unhandled buffer is %s\n", (char*)request->unhandled_buffer);
				printf("request content length %d\n", request->content_length);
				printf("request content: %s\n", request->message_body);
			}
            return request;
		}
	}
    //TODO Handle Malformed Requests
	YY_BUFFER_STATE bufferstate = yy_create_buffer(NULL, 8192);
	yy_switch_to_buffer(bufferstate);
    printf("Parsing Failed\n");
	return NULL;
}

int get_header_length(char *buffer, int size) {
	enum {
		STATE_START = 0, STATE_CR, STATE_CRLF, STATE_CRLFCR, STATE_CRLFCRLF
	};

	int state;
	size_t header_offset = 0;
	char ch;

	int crlf_num = 0;

	state = STATE_START;
	while (state != STATE_CRLFCRLF) {
		char expected = 0;

		if (header_offset == size) {
			break;
		}
		

		ch = buffer[header_offset++];

		switch (state) {
		case STATE_START:
		case STATE_CRLF:
			expected = '\r';
			break;
		case STATE_CR:
		case STATE_CRLFCR:
			crlf_num += 1;
			expected = '\n';
			break;
		default:
			state = STATE_START;
			continue;
		}

		if (ch == expected)
			state++;
		else
			state = STATE_START;

	}

	if (state == STATE_CRLFCRLF) {
		return header_offset;
	}
	return -1;
	
}

