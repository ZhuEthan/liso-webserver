SRC_DIR := src
OBJ_DIR := obj
UTIL_DIR := obj/util
EXP_DIR := src/multiplexing_server
# all src files
SRC := $(wildcard $(SRC_DIR)/*.c)
EXP := $(wildcard $(EXP_DIR)/*.c)
# all objects
OBJ := $(OBJ_DIR)/y.tab.o $(OBJ_DIR)/lex.yy.o $(OBJ_DIR)/parse.o $(OBJ_DIR)/example.o

#Server Objects
SERVER_OBJ := $(OBJ_DIR)/y.tab.o $(OBJ_DIR)/lex.yy.o $(OBJ_DIR)/parse.o $(OBJ_DIR)/echo_server.o

EXP_OBJ := $(OBJ_DIR)/y.tab.o $(OBJ_DIR)/lex.yy.o $(OBJ_DIR)/parse.o $(OBJ_DIR)/echoservers.o $(OBJ_DIR)/csapp.o $(UTIL_DIR)/log.o

#Client Objects
CLIENT_OBJ := $(OBJ_DIR)/y.tab.o $(OBJ_DIR)/lex.yy.o $(OBJ_DIR)/parse.o $(OBJ_DIR)/echo_client.o

# all binaries
BIN := example echo_server echo_client
# C compiler
CC  := gcc
# C PreProcessor Flag
CPPFLAGS := -Iinclude
# compiler flags
CFLAGS   := -g -Wall
# DEPS = parse.h y.tab.h

default: all
all : example echo_server echo_client

example: $(OBJ)
	$(CC) $^ -o $@

$(SRC_DIR)/lex.yy.c: $(SRC_DIR)/lexer.l
	flex -o $@ $^

$(SRC_DIR)/y.tab.c: $(SRC_DIR)/parser.y
	yacc -d $^
	mv y.tab.c $@
	mv y.tab.h $(SRC_DIR)/y.tab.h

$(OBJ_DIR)/echoservers.o: $(EXP_DIR)/echoservers.c $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(UTIL_DIR)/%.o: $(EXP_DIR)/util/%.c $(OBJ_DIR) $(UTIL_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/csapp.o: $(EXP_DIR)/csapp.c $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

echo_server: $(SERVER_OBJ) #$(OBJ_DIR)/echo_server.o 
	$(CC) -Werror $^ -o $@

select_server: $(EXP_OBJ)
	$(CC) -Werror $^ -o $@

echo_client: $(CLIENT_OBJ)
	$(CC) -Werror $^ -o $@

$(OBJ_DIR):
	mkdir $@

$(UTIL_DIR):
	mkdir $@

clean:
	$(RM) $(OBJ) $(BIN) $(SRC_DIR)/lex.yy.c $(SRC_DIR)/y.tab.*
	$(RM) -r $(OBJ_DIR)
