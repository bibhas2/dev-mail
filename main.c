#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "../SockFramework/socket-framework.h"

Server *create_smtp_server();
Server *create_pop3_server();

int main() {
	Server *smtp = create_smtp_server();
    Server *pop3 = create_pop3_server();
    EventLoop loop;

	loopInit(&loop);

    loop.idle_timeout = 15;

    loopAddServer(&loop, smtp);
    loopAddServer(&loop, pop3);
    
    loopStart(&loop);

	deleteServer(smtp);
    deleteServer(pop3);
}