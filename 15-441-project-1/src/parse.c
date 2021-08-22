#include "parse.h"

//Request * parse(char *buffer, int size, int socketFd) {
	//Request* request = parse_header(buffer, size, socketFd);
	
	

	//if (request->content_length > 0) {
		//TODO: move the ptr back to the end of the header and then read content again. if reading lenth is less than destination length, return false
			//request->message_body_size = MIN(request->content_length, size-request->header_offset);
			//memcpy(request->message_body, buffer+request->header_offset, request->message_body_size);

			//request->unhandled_buffer_size = MAX(size-request->header_offset-(request->message_body_size), 0);
			//memcpy(request->unhandled_buffer, buffer+request->header_offset+request->message_body_size, request->unhandled_buffer_size);
			//memcpy(request->unhandled_buffer, buffer+offset+request->content_length, size-offset-(request->content_length));
			//printf("request unhandled buffer is %s\n", (char*)request->unhandled_buffer);
			//printf("request content length %d\n", request->content_length);
			//printf("request content: %s\n", request->message_body);
		//}	
//}

/**
* Given a char buffer returns the parsed request headers
*/
Request *parse_header(char *buffer, int size, int socketFd) {
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
		printf("request->header number is %d\n", crlf_num-2);
        request->headers = (Request_header *) malloc(sizeof(Request_header)*MAX(0, crlf_num-2));
		set_parsing_options(buf, i, request);

		if (yyparse() == SUCCESS) {
			printf("header_offset %d\n", header_offset);
			request->header_offset = header_offset;
            return request;
		}
	}
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

