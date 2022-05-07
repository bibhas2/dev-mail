#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include "../Cute/String.h"
#include "../Cute/Array.h"
#include "../SockFramework/socket-framework.h"
#include <sys/mman.h>
#include <dirent.h>
#include <fcntl.h>

typedef enum {
	STATE_NONE,
	STATE_READ_CMD,
	STATE_WRITE_LIST,
	STATE_WRITE_MSG_HEADER,
	STATE_WRITE_MSG,
	STATE_BYE
} ParseState;

typedef struct {
	int fd;
	size_t file_size;
	void *file_map;
	char file_name[256];
} MessageRecord;

typedef struct _POP3State {
	ParseState parse_state;
	char read_buffer[1024];
	char write_buffer[1024];
	Array *msg_list;
	size_t msg_index;
} POP3State;

int counter = 0;
char *MAIL_DIR = "mail";

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

MessageRecord *
new_message_record(char *file_name) {
	MessageRecord *rec = malloc(sizeof(MessageRecord));

	rec->fd = -1;
	rec->file_map = NULL;

	snprintf(rec->file_name, sizeof(rec->file_name), "%s/%s", MAIL_DIR, file_name);

	struct stat st;

	int status = stat(rec->file_name, &st);

	assert(status == 0);

	rec->file_size = st.st_size;

	return rec;
}

void
release_message_rec_resources(MessageRecord *rec) {
	if (rec->file_map != NULL) {
		int status = munmap(rec->file_map, rec->file_size);
		assert(status == 0);
		rec->file_map = NULL;
	}
	if (rec->fd > 0) {
		close(rec->fd);

		rec->fd = -1;
	}
}

void
acquire_message_rec_resources(MessageRecord *rec) {
	release_message_rec_resources(rec);

	rec->fd = open(rec->file_name, O_RDONLY);
	assert(rec->fd > 0);

	rec->file_map = mmap(
			NULL, rec->file_size, 
			PROT_READ, MAP_SHARED,
			rec->fd, 0);

	assert(rec->file_map != MAP_FAILED);
}

void
clear_message_list(POP3State *pop3) {
	for (int i = 0; i < pop3->msg_list->length; ++i) {
		MessageRecord *rec = arrayGet(pop3->msg_list, i);

		release_message_rec_resources(rec);

		free(rec);

		arraySet(pop3->msg_list, i, NULL);
	}

	pop3->msg_list->length = 0;
}

void
load_message_list(POP3State *pop3) {
	clear_message_list(pop3);

	DIR *dirp = opendir(MAIL_DIR);

	if (dirp == NULL) {
		perror("Can not open mail folder.");

		return;
	}

	struct dirent *de;

	while ((de = readdir(dirp)) != NULL) {
		if (de->d_type != DT_REG) {
			continue;
		}

		MessageRecord *rec = new_message_record(de->d_name);

		arrayAdd(pop3->msg_list, rec);
	}
	
	closedir(dirp);
}

void on_connect(Server *state, Client *cli_state) {
	_info("Client connected %d", cli_state->fd);

	POP3State *pop3 = (POP3State*) malloc(sizeof(POP3State));

	pop3->parse_state = STATE_NONE;
	pop3->msg_list = newArray(10);
	
	cli_state->data = pop3;

	write_to_client(pop3, cli_state, "+OK POP3 example.com server ready\r\n");

	//Start reading request
	pop3->parse_state = STATE_READ_CMD;
	read_from_client(pop3, cli_state);
}

void on_disconnect(Server *state, Client *cli_state) {
	_info("Client disconnected %d", cli_state->fd);

	POP3State *pop3 = (POP3State*) cli_state->data;

	clear_message_list(pop3);
	deleteArray(pop3->msg_list);

	free(pop3);
}

bool starts_with(const char *str, const char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

void
process_command(Server *state, POP3State *pop3, Client *cli_state, size_t line_size) {
	_info("C: %s", pop3->read_buffer);

	if (starts_with(pop3->read_buffer, "USER")) {
		write_to_client(pop3, cli_state, "+OK User name accepted, password please\r\n");
		read_from_client(pop3, cli_state);
	} else if (starts_with(pop3->read_buffer, "PASS")) {
		write_to_client(pop3, cli_state, "+OK Mailbox open\r\n");
		read_from_client(pop3, cli_state);
	} else if (starts_with(pop3->read_buffer, "STAT")) {
		load_message_list(pop3);
		size_t sz = 0;

		for (int i = 0; i < pop3->msg_list->length; ++i) {
			MessageRecord *rec = arrayGet(pop3->msg_list, i);

			sz += rec->file_size;
		}

		char reply[256];

		snprintf(reply, sizeof(reply), "+OK %ld %ld\r\n", pop3->msg_list->length, sz);

		write_to_client(pop3, cli_state, reply);
		read_from_client(pop3, cli_state);

		clear_message_list(pop3);
	} else if (starts_with(pop3->read_buffer, "LIST")) {
		load_message_list(pop3);
		pop3->parse_state = STATE_WRITE_LIST;
		pop3->msg_index = 0;
		write_to_client(pop3, cli_state, "+OK Mailbox scan listing follows\r\n");
	} else if (starts_with(pop3->read_buffer, "QUIT")) {
		write_to_client(pop3, cli_state, "+OK Bye\r\n");
		pop3->parse_state = STATE_BYE;
	} else if (starts_with(pop3->read_buffer, "RETR")) {
		size_t n;

		sscanf(pop3->read_buffer, "%*s %ld", &n);

		if (n >= pop3->msg_list->length) {
			write_to_client(pop3, cli_state, "-ERR\r\n");
		} else {
			pop3->msg_index = n - 1;

			char reply[256];
			MessageRecord *rec = arrayGet(pop3->msg_list, pop3->msg_index);

			snprintf(reply, sizeof(reply), "+OK %ld octate\r\n", rec->file_size);

			write_to_client(pop3, cli_state, reply);

			pop3->parse_state = STATE_WRITE_MSG_HEADER;
		}
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
			size_t line_size = cli_state->read_completed;
			//We have reached the end of a line.
			//Don't read any more
			clientCancelRead(cli_state);

			process_command(state, pop3, cli_state, line_size);
		}
	}
}

void on_write_completed(Server *state, Client *cli_state) {
	POP3State *pop3 = (POP3State*) cli_state->data;

	if (pop3->parse_state == STATE_BYE) {
		serverDisconnect(state, cli_state);
	} else if (pop3->parse_state == STATE_WRITE_LIST) {
		if (pop3->msg_index == pop3->msg_list->length) {
			//We are done writing the list
			write_to_client(pop3, cli_state, ".\r\n");

			//Start reading again.
			pop3->parse_state = STATE_READ_CMD;
			read_from_client(pop3, cli_state);
		} else {
			char reply[256];
			MessageRecord *rec = arrayGet(pop3->msg_list, pop3->msg_index);
			
			snprintf(reply, sizeof(reply), "%ld %ld\r\n", pop3->msg_index + 1, rec->file_size);

			pop3->msg_index += 1;

			write_to_client(pop3, cli_state, reply);
		} 
	} else if (pop3->parse_state == STATE_WRITE_MSG_HEADER) {
		MessageRecord *rec = arrayGet(pop3->msg_list, pop3->msg_index);

		acquire_message_rec_resources(rec);

		pop3->parse_state = STATE_WRITE_MSG;

		clientScheduleWrite(cli_state, rec->file_map, rec->file_size);
	} else if (pop3->parse_state == STATE_WRITE_MSG) {
		MessageRecord *rec = arrayGet(pop3->msg_list, pop3->msg_index);

		release_message_rec_resources(rec);

		write_to_client(pop3, cli_state, "\r\n.\r\n");

		//Start reading again.
		pop3->parse_state = STATE_READ_CMD;
		read_from_client(pop3, cli_state);
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
