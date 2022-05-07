#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include "../Cute/String.h"
#include "../SockFramework/socket-framework.h"

typedef enum {
	STATE_NONE,
	STATE_READ_CMD,
	STATE_BYE
} ParseState;

typedef struct _POP3State {
	ParseState parse_state;
	char read_buffer[1024];
	char write_buffer[1024];
} POP3State;

int counter = 0;

void
init_server(Server* state) {
	_info("Server loop is starting");
}

void
write_to_client(POP3State *pop3, Client *cli_state, char* line) {
	_info("S: %s", line);

	char *end = stpncpy(pop3->write_buffer, line, sizeof(pop3->write_buffer));

	clientScheduleWrite(cli_state, pop3->write_buffer, end - pop3->write_buffer);
}

void
read_from_client(POP3State *pop3, Client *cli_state) {
	memset(pop3->read_buffer, '\0', sizeof(pop3->read_buffer));
	
	int status = clientScheduleRead(cli_state, pop3->read_buffer, 
		sizeof(pop3->read_buffer));

	assert(status == 0);
}

void on_connect(Server *state, Client *cli_state) {
	_info("Client connected %d", cli_state->fd);

	POP3State *pop3 = (POP3State*) malloc(sizeof(POP3State));

	pop3->parse_state = STATE_NONE;
	
	cli_state->data = pop3;

	write_to_client(pop3, cli_state, "+OK POP3 example.com server ready\r\n");

	//Start reading request
	pop3->parse_state = STATE_READ_CMD;
	read_from_client(pop3, cli_state);
}

void on_disconnect(Server *state, Client *cli_state) {
	_info("Client disconnected %d", cli_state->fd);

	POP3State *pop3 = (POP3State*) cli_state->data;

	free(pop3);
}

bool starts_with(const char *str, const char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

void
process_command(Server *state, POP3State *pop3, Client *cli_state) {
	_info("C: %s", pop3->read_buffer);

	if (starts_with(pop3->read_buffer, "USER")) {
		write_to_client(pop3, cli_state, "+OK User name accepted, password please\r\n");
		read_from_client(pop3, cli_state);
	} else if (starts_with(pop3->read_buffer, "PASS")) {
		write_to_client(pop3, cli_state, "+OK Mailbox open\r\n");
		read_from_client(pop3, cli_state);
	} else if (starts_with(pop3->read_buffer, "STAT")) {
		write_to_client(pop3, cli_state, "+OK 0 0\r\n");
		read_from_client(pop3, cli_state);
	} else if (starts_with(pop3->read_buffer, "LIST")) {
		write_to_client(pop3, cli_state, "+OK Mailbox scan listing follows\r\n");
		read_from_client(pop3, cli_state);
	} else if (starts_with(pop3->read_buffer, "QUIT")) {
		write_to_client(pop3, cli_state, "+OK Bye\r\n");
		pop3->parse_state = STATE_BYE;
	} else {
		_info("Unknown command.");
		write_to_client(pop3, cli_state, "-ERR\r\n");
		read_from_client(pop3, cli_state);
	}
}

void on_read(Server *state, Client *cli_state, char *buff, size_t length) {
	POP3State *pop3 = (POP3State*) cli_state->data;

	if (pop3->parse_state == STATE_READ_CMD) {
		if (buff[length - 1] == '\n') {
			//We have reached the end of a line.
			//Don't read any more
			clientCancelRead(cli_state);

			process_command(state, pop3, cli_state);
		}
	}
}

void on_write_completed(Server *state, Client *cli_state) {
	POP3State *pop3 = (POP3State*) cli_state->data;

	if (pop3->parse_state == STATE_BYE) {
		serverDisconnect(state, cli_state);
	}
}

void on_read_completed(Server *state, Client *cli_state) {
	//If we are still parsing request, keep reading
	POP3State *pop3 = (POP3State*) cli_state->data;
}

int main() {
	Server *state = newServer(1010);

	state->on_loop_start = init_server;
	state->on_client_connect = on_connect;
	state->on_client_disconnect = on_disconnect;
	state->on_read = on_read;
	state->on_read_completed = on_read_completed;
	//state->on_write = on_write;
	state->on_write_completed = on_write_completed;

	serverStart(state);

	deleteServer(state);
}
