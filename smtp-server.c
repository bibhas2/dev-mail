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
	STATE_READ_DATA
} ParseState;

/*
From: https://www.ibm.com/docs/en/zos/2.3.0?topic=set-smtp-commands
The SMTP command lines must not exceed 510 characters in length.
Data lines must not exceed 998 characters in length.
*/
typedef struct _SMTPState {
	ParseState parse_state;
	char read_buffer[1024];
	char write_buffer[1024];
	String *data;
} SMTPState;

int counter = 0;

void
init_server(Server* state) {
	_info("Server loop is starting");

	if (mkdir("mail", 0777) != 0 && errno != EEXIST) {
		perror("Failed to create mail/ folder.");
		exit(1);
	}
}

void
write_to_client(SMTPState *smtp, Client *cli_state, char* line) {
	_info("S: %s", line);

	char *end = stpncpy(smtp->write_buffer, line, sizeof(smtp->write_buffer));

	clientScheduleWrite(cli_state, smtp->write_buffer, end - smtp->write_buffer);
}

void
read_from_client(SMTPState *smtp, Client *cli_state) {
	memset(smtp->read_buffer, '\0', sizeof(smtp->read_buffer));
	
	int status = clientScheduleRead(cli_state, smtp->read_buffer, 
		sizeof(smtp->read_buffer));

	assert(status == 0);
}

void on_connect(Server *state, Client *cli_state) {
	_info("Client connected %d", cli_state->fd);

	SMTPState *smtp = (SMTPState*) malloc(sizeof(SMTPState));

	smtp->parse_state = STATE_NONE;
	smtp->data = newStringWithCapacity(1024);
	cli_state->data = smtp;

	write_to_client(smtp, cli_state, "220 example.com\r\n");

	//Start reading request
	smtp->parse_state = STATE_READ_CMD;
	read_from_client(smtp, cli_state);
}

void on_disconnect(Server *state, Client *cli_state) {
	_info("Client disconnected %d", cli_state->fd);

	SMTPState *smtp = (SMTPState*) cli_state->data;

	deleteString(smtp->data);

	free(smtp);
}

bool starts_with(const char *str, const char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

void
process_command(SMTPState *smtp, Client *cli_state) {
	_info("C: %s", smtp->read_buffer);

	if (starts_with(smtp->read_buffer, "HELO")) {
		write_to_client(smtp, cli_state, "250 Ok\r\n");
		read_from_client(smtp, cli_state);
	} else if (starts_with(smtp->read_buffer, "EHLO")) {
		write_to_client(smtp, cli_state, "250-dev-smtp\r\n250-8BITMIME\r\n250-AUTH LOGIN\r\n250 Ok\r\n");
		read_from_client(smtp, cli_state);
	} else if (starts_with(smtp->read_buffer, "MAIL FROM:")) {
		write_to_client(smtp, cli_state, "250 Ok\r\n");
		read_from_client(smtp, cli_state);
	} else if (starts_with(smtp->read_buffer, "RCPT TO:")) {
		write_to_client(smtp, cli_state, "250 Ok\r\n");
		read_from_client(smtp, cli_state);
	} else if (starts_with(smtp->read_buffer, "QUIT")) {
		write_to_client(smtp, cli_state, "221 Bye\r\n");
		read_from_client(smtp, cli_state);
	} else if (starts_with(smtp->read_buffer, "DATA")) {
		write_to_client(smtp, cli_state, "354 Send message, end with a \".\" on a line by itself\r\n");
		smtp->parse_state = STATE_READ_DATA;
		read_from_client(smtp, cli_state);
	} else {
		_info("Unknown command.");
		write_to_client(smtp, cli_state, "250 Ok\r\n");
		read_from_client(smtp, cli_state);
	}
}

void
process_data(SMTPState *smtp, Client *cli_state) {
	if (smtp->data->length < 5) {
		return;
	}
	//See if we got \n.\n
	if (stringGetChar(smtp->data, smtp->data->length - 5) == '\r' &&
		stringGetChar(smtp->data, smtp->data->length - 4) == '\n' &&
		stringGetChar(smtp->data, smtp->data->length - 3) == '.' &&
		stringGetChar(smtp->data, smtp->data->length - 2) == '\r' &&
		stringGetChar(smtp->data, smtp->data->length - 1) == '\n') {
		
		char path[256];

		sprintf(path, "mail/%d.eml", ++counter);

		FILE *mail_file = fopen(path, "w");

		assert(mail_file != NULL);

		//Truncate the message end indicators
		fwrite(smtp->data->buffer, smtp->data->length - 5, 1, mail_file);
		fclose(mail_file);

		smtp->parse_state = STATE_READ_CMD;
		smtp->data->length = 0;
		clientCancelRead(cli_state);
		write_to_client(smtp, cli_state, "250 OK\r\n");
		read_from_client(smtp, cli_state);
	}
}

void on_read(Server *state, Client *cli_state, char *buff, size_t length) {
	SMTPState *smtp = (SMTPState*) cli_state->data;

	if (smtp->parse_state == STATE_READ_CMD) {
		if (buff[length - 1] == '\n') {
			//We have reached the end of a line.
			//Don't read any more
			clientCancelRead(cli_state);

			process_command(smtp, cli_state);
		}
	} else if (smtp->parse_state == STATE_READ_DATA) {
		stringAppendBuffer(smtp->data, buff, length);

		process_data(smtp, cli_state);
	}
}

void on_write_completed(Server *state, Client *cli_state) {
	SMTPState *smtp = (SMTPState*) cli_state->data;
}

void on_read_completed(Server *state, Client *cli_state) {
	//If we are still parsing request, keep reading
	SMTPState *smtp = (SMTPState*) cli_state->data;

	if (smtp->parse_state == STATE_READ_DATA) {
		read_from_client(smtp, cli_state);
	}
}

int main() {
	Server *state = newServer(2525);

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
