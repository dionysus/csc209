#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include "socket.h"

#ifndef PORT
    #define PORT 55160
#endif

#define LISTEN_SIZE 5
#define WELCOME_MSG "Welcome to CSC209 Twitter! Enter your username: "
#define SEND_MSG "send"
#define SHOW_MSG "show"
#define FOLLOW_MSG "follow"
#define UNFOLLOW_MSG "unfollow"
#define BUF_SIZE 256
#define MSG_LIMIT 8
#define FOLLOW_LIMIT 5

struct client {
    int fd;
    struct in_addr ipaddr;
    char username[BUF_SIZE];
    char message[MSG_LIMIT][BUF_SIZE];
    struct client *following[FOLLOW_LIMIT]; // Clients this user is following
    struct client *followers[FOLLOW_LIMIT]; // Clients who follow this user
    char inbuf[BUF_SIZE]; // Used to hold input from the client
    char *in_ptr; // A pointer into inbuf to help with partial reads
    struct client *next;
};


// Provided functions. 
void add_client(struct client **clients, int fd, struct in_addr addr);
void remove_client(struct client **clients, int fd);
void announce(struct client *active_clients, char *s);
void activate_client(struct client *c, 
    struct client **active_clients_ptr, struct client **new_clients_ptr);

void _write(int fd, const void *msg, size_t count);

// TODO:
void follow(struct client *user, struct client *username); //! Not sure yet

//! WRAPPER FUNCTIONS ----------------------------------------------------------

/*
 * Calls malloc() and check for errors
 */
void *_malloc(int size) {
	void *ptr;

	if ((ptr = malloc(size)) == NULL) {
		perror("malloc");
		exit(1);
	}
	return ptr;
}

/*
 * Calls write() and check for errors
 */
void _write(int fd, const void *msg, size_t count) {
    if (write(fd, msg, count) != count) {
        perror("write");
        exit(1);
    }
}

//! Tester Functions ----------------------------------------------------------

void print_list(struct client *clients, const char *s){
    struct client *curr_client = clients;
    printf("----------\n%s\n", s);
    int i = 1;
    if (curr_client == NULL){
        printf("    None!\n");
    }
    else {
        while(curr_client != NULL){
            printf("    Client%d: (%d) %s \n", i, curr_client->fd, curr_client->username);
            i++;
            curr_client = curr_client->next;
        }
    }
    printf("----------\n");
}

//! Helper Functions ----------------------------------------------------------

// The set of socket descriptors for select to monitor.
// This is a global variable because we need to remove socket descriptors
// from allset when a write to a socket fails. 
fd_set allset;

/* 
 * Create a new client, initialize it, and add it to the head of the linked
 * list.
 */
void add_client(struct client **clients, int fd, struct in_addr addr) {
    struct client *p = _malloc(sizeof(struct client));

    printf("Adding client %s\n", inet_ntoa(addr));
    p->fd = fd;
    p->ipaddr = addr;
    p->username[0] = '\0';
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *clients; // placed at front of linked list

    // initialize messages to empty strings
    for (int i = 0; i < MSG_LIMIT; i++) {
        p->message[i][0] = '\0';
    }

    // new client p is returned as head of clients
    *clients = p;
}

/* 
 * Remove client from the linked list and close its socket.
 * Also, remove socket descriptor from allset.
 */
void remove_client(struct client **clients, int fd) {
    struct client **p;

    for (p = clients; *p && (*p)->fd != fd; p = &(*p)->next)
        ;

    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        // TODO: Remove the client from other clients' following/followers
        // lists

        // Remove the client
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        FD_CLR((*p)->fd, &allset);
        close((*p)->fd);
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, 
            "Trying to remove fd %d, but I don't know about it\n", fd);
    }
}

// Move client c from new_clients list to active_clients list. 
void activate_client(struct client *c, 
    struct client **active_clients_ptr, struct client **new_clients_ptr) {
        // TODO: check this one!

        //! remove client from new_client list
            // navigate linked list of new_clients_ptr to find next = client
                //next = client.next
            struct client *curr_client  = *new_clients_ptr;

            // c is at the head
            if(curr_client-> fd == c->fd) {
                *new_clients_ptr = c->next;
            } 
            else {
                while ((curr_client->next)->fd != c->fd) {
                    curr_client = curr_client->next;
                }
                curr_client->next = c->next;
            }
        //! add client to active_clients (front of LL)
            // set front of active_clients_ptr to c
            c->next = *active_clients_ptr;
            *active_clients_ptr = c;
            print_list(*new_clients_ptr, "New Clients");
            print_list(*active_clients_ptr, "Active Clients");
    }

/*
 * Send the message in s to all clients in active_clients.
 */
void announce(struct client *active_clients, char *s) {
    struct client *curr_client = active_clients;
    for ( ; curr_client != NULL; curr_client = curr_client->next ) {
        _write(curr_client->fd, s, strlen(s));
    }
    printf("Announcement: %s\n", s);
};


int checkUsername(struct client *active_clients, char* s) {
    printf("checkUsername: (%s)\n", s);
    // find duplicates
    for ( struct client *curr = active_clients ; curr != NULL; curr = curr->next ) {
        printf("checkUsername: against (%s)\n", curr->username);
        if (strcmp(s, curr->username) == 0) {
            printf("checkUsername: (%s) duplicate found!\n", s);
            return 0;
        }
    }
    printf("checkUsername: (%s) no match!\n", s);
    return 1;
    
}

void removeNewline(char *s, int count){
    for (int i = 0; i < count; i++){
        if ((s[i] == '\r' && s[i+1] == '\n') || s[i] == '\n'){
            s[i] = '\0';
        }
    }
}
//TODO: FUNCTIONS --------------------------
/* 
 * Create a follow relationship between user and target username
 * Add target username to user's following list
 * Add user to target username's follower list
 * iff both following/follower list is < FOLLOW_LIMIT
 */
void follow(struct client *user, struct client *username);

// ! Main ----------------------------------------------------------------------

int main (int argc, char **argv) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct sockaddr_in q;
    fd_set rset;

    // If the server writes to a socket that has been closed, the SIGPIPE
    // signal is sent and the process is terminated. To prevent the server
    // from terminating, ignore the SIGPIPE signal. 
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // A list of active clients (who have already entered their names). 
    struct client *active_clients = NULL;

    // A list of clients who have not yet entered their names. This list is
    // kept separate from the list of active clients, because until a client
    // has entered their name, they should not issue commands or 
    // or receive announcements. 
    struct client *new_clients = NULL;

    struct sockaddr_in *server = init_server_addr(PORT);
    int listenfd = set_up_server_socket(server, LISTEN_SIZE);

    // Initialize allset and add listenfd to the set of file descriptors
    // passed into select 
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    // maxfd identifies how far into the set to search
    maxfd = listenfd;

    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            perror("select");
            exit(1);
        } else if (nready == 0) {
            continue;
        }

        // check if a new client is connecting
        if (FD_ISSET(listenfd, &rset)) {
            printf("A new client is connecting\n");
            clientfd = accept_connection(listenfd, &q);

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("Connection from %s\n", inet_ntoa(q.sin_addr));
            add_client(&new_clients, clientfd, q.sin_addr);
            char *greeting = WELCOME_MSG;
            if (write(clientfd, greeting, strlen(greeting)) == -1) {
                fprintf(stderr, 
                    "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                remove_client(&new_clients, clientfd);
            }
        }

        // Check which other socket descriptors have something ready to read.
        // The reason we iterate over the rset descriptors at the top level and
        // search through the two lists of clients each time is that it is
        // possible that a client will be removed in the middle of one of the
        // operations. This is also why we call break after handling the input.
        // If a client has been removed, the loop variables may no longer be 
        // valid.
        int cur_fd, handled;
        for (cur_fd = 0; cur_fd <= maxfd; cur_fd++) {
            if (FD_ISSET(cur_fd, &rset)) {
                handled = 0;

                // Check if any new clients are entering their names
                for (p = new_clients; p != NULL; p = p->next) {
                    if (cur_fd == p->fd) {
                        // TODO: handle input from a new client who has not yet
                        // entered an acceptable name

                        printf("--processing new client--\n");
                        
                        memset(p->inbuf, '\0', BUF_SIZE);
                        int num_read = read(cur_fd, &p->inbuf, BUF_SIZE);

                        removeNewline(p->inbuf, num_read);
                            printf("inbuf: %s\n", p->inbuf);

                        char msg[BUF_SIZE];
                            memset(msg, '\0', BUF_SIZE);

                        //TODO: complete check and activate
                        if (checkUsername(active_clients, p->inbuf) == 1) {
                            memset(p->username, '\0', BUF_SIZE);
                            strncpy(p->username, p->inbuf, strlen(p->inbuf));
                                // p->username[strlen(p->username)] = '\0';
                            // printf("Connected: %s\n", p->username);
                            activate_client(p, &active_clients, &new_clients);
                            // printf("Activated: %s\n", p->username);
                            // Announce joined
                            // char msg[strlen(buf) + strlen(JOINED_MSG) + 1];
                                strncpy(msg, p->username, strlen(p->username));
                                    // printf("msg1: %s\n", msg);
                                strcat(msg, " has joined!\n");
                                    // printf("msg3: %s\n", msg);
                            announce(active_clients, msg);
                        }

                        handled = 1;
                        break;
                    }
                }

                if (!handled) {
                    // Check if this socket descriptor is an active client
                    for (p = active_clients; p != NULL; p = p->next) {
                        if (cur_fd == p->fd) {
                            // TODO: handle input from an active client
                            break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
