#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <signal.h>
#include "../SockFramework/socket-framework.h"
#define SMTP_PORT 2525
#define POP3_PORT 1100

Server *create_smtp_server(int port);
Server *create_pop3_server(int port);

static int quiet = 0;

char* get_next_arg(int argc, char *argv[], int *pos) {
    if (*pos > (argc - 1)) {
        return NULL;
    }

    *pos += 1;

    return argv[*pos];
}

void get_next_arg_int(int argc, char *argv[], int* pos, int* value) {
    char *p = get_next_arg(argc, argv, pos);

    if (p != NULL) {
        *value = atoi(p);
    }
}

void _info(const char* format, ...) {
    if (quiet == 1) {
        return;
    }

    va_list argptr;

    va_start(argptr, format);

    vprintf(format, argptr);

    va_end(argptr);
}

EventLoop loop;

void handle_term(int signum) {
    loopEnd(&loop);
}

int main(int argc, char *argv[]) {
    int pos = 0;
    char* arg = NULL;
    int pop3_port = POP3_PORT;
    int smtp_port = SMTP_PORT;

    while ((arg = get_next_arg(argc, argv, &pos)) != NULL) {
        if (strcmp(arg, "--pop3-port") == 0) {
            get_next_arg_int(argc, argv, &pos, &pop3_port);
        } else if (strcmp(arg, "--smtp-port") == 0) {
            get_next_arg_int(argc, argv, &pos, &smtp_port);
        } else if (strcmp(arg, "--quiet") == 0) {
            quiet = 1;
        }
    }

    signal(SIGTERM, handle_term);

	Server *smtp = create_smtp_server(smtp_port);
    Server *pop3 = create_pop3_server(pop3_port);

	loopInit(&loop);

    loop.idle_timeout = 15;

    loopAddServer(&loop, smtp);
    loopAddServer(&loop, pop3);
    
    loopStart(&loop);

    _info("Received SIGTERM. Shutting down.\n");

	deleteServer(smtp);
    deleteServer(pop3);
}