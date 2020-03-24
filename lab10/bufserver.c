#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "socket.h"

#ifndef PORT
  #define PORT 30000
#endif

#define BUFSIZE 30

int find_network_newline(const char *buf, int n);


int main() {
    // This line causes stdout not to be buffered.
    // Don't change this! Necessary for autotesting.
    setbuf(stdout, NULL);

    struct sockaddr_in *self = init_server_addr(PORT);
    int listenfd = set_up_server_socket(self, 5);

    while (1) {
        int fd = accept_connection(listenfd);
        if (fd < 0) {
            continue;
        }

        // Receive messages
        char buf[BUFSIZE] = {'\0'};
        int inbuf = 0;           // How many bytes currently in buffer?
        int room = sizeof(buf);  // How many bytes remaining in buffer?
        char *after = buf;       // Pointer to position after the data in buf

        int nbytes;
        while ((nbytes = read(fd, after, room)) > 0) {

            inbuf += nbytes;
            int where;

            while ((where = find_network_newline(buf, inbuf)) > 0) {

                buf[where-2] = '\0'; // returned /n + 1, so need to go back 2
                printf("Next message: %s\n", buf);

                inbuf -= where;
                memmove(buf, buf + where, inbuf);

            }

            after = buf + inbuf;
            room = sizeof(buf) - inbuf;

        }
        close(fd);
    }

    free(self);
    close(listenfd);
    return 0;
}


/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int n) {

    for (int i = 0; i < n; i++) {
        if (buf[i] == '\n' && buf[i-1] == '\r'){
            return i + 1;
        }
    }
    return -1;
}
