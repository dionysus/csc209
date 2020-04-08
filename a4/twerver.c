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
#define JOINED_MSG " has just joined!\r\n"
#define INVUSR_MSG "Invalid Username! Enter a new username: "
#define SEND_MSG "send"
#define SHOW_MSG "show"
#define FOLLOW_MSG "follow"
#define UNFOLLOW_MSG "unfollow"
#define QUIT_MSG "quit"
#define BUF_SIZE 256
#define MSG_LIMIT 8
#define MSG_LENGTH 140
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


// Helper Functions
void add_client(struct client **clients, int fd, struct in_addr addr);
void remove_client(struct client **clients, int fd);
void announce(struct client *active_clients, char *s);
void activate_client(struct client *c, 
    struct client **active_clients_ptr, struct client **new_clients_ptr);
int check_username(struct client *active_clients, char* s);
int max_cmd_length();
int slice_command(const char *s, int n, char *command);
int find_word(const char *s, int n);
int find_network_newline(const char *buf, int n);
void clear_inbuf(struct client *p);

// Wrapper Functions
void _write(int fd, const void *msg, size_t count);
void *_malloc(int size);

// Command Functions
int parse_command(struct client *p, struct client **active_clients);
void follow(struct client *user, char *target, struct client **active_clients);
void unfollow(struct client *p, char *target, struct client **active_clients);
void send_message(struct client *p);
void show(struct client *p);

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
        // exit(1);
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

void printFollowers(struct client *p) {
    printf("\n-- Followers of %s --\n", p->username);
    for (int i = 0; i < FOLLOW_LIMIT - 1; i++) {
        if (p->followers[i] != NULL){
            printf("User %d: %s\n", i, (p->followers[i])->username);
        }
    }
    printf("----------------------\n\n");
}

void printFollowing(struct client *p) {
    printf("-- %s is Following --\n", p->username);
    for (int i = 0; i < FOLLOW_LIMIT - 1; i++) {
        if (p->following[i] != NULL){
            printf("User %d: %s\n", i, (p->following[i])->username);
        }
    }
    printf("----------------------\n\n");
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

        struct client *q;
        for (q = *clients; (q)->fd != fd; q = q->next);

        // Remove Leaver from users' Following lists
        for (int i = 0; i < FOLLOW_LIMIT - 1; i++) {
            if (q->followers[i] != NULL){
                for (int j = 0; j < FOLLOW_LIMIT - 1; j++) {
                    if (((q->followers[i])->following)[j] == q){
                        (q->followers[i])->following[j] = NULL;
                        // printf("Removed %s from %s's following list\n", q->username, (q->followers[i])->username);
                        break;
                    }
                }
            }
        }
        // Remove Leaver from user's Followers lists
        for (int i = 0; i < FOLLOW_LIMIT - 1; i++) {
            if (q->following[i] != NULL){
                for (int j = 0; j < FOLLOW_LIMIT - 1; j++) {
                    if (((q->following[i])->followers)[j] == q){
                        (q->following[i])->followers[j] = NULL;
                        // printf("Removed %s from %s's followers list\n", q->username, (q->followers[i])->username);
                        break;
                    }
                }
            }
        }

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

/* 
 * Move client c from new_clients list to active_clients list. 
 */
void activate_client(struct client *c, 
    struct client **active_clients_ptr, struct client **new_clients_ptr) {

    //! remove client from new_client list
        struct client *curr_client  = *new_clients_ptr;

        // c is at the head
        if(curr_client->fd == c->fd) {
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
 * Send a message to every client in a linked list
 */
void announce(struct client *active_clients, char *s) {
    struct client *curr_client = active_clients;
    for ( ; curr_client != NULL; curr_client = curr_client->next ) {
        _write(curr_client->fd, s, strlen(s));
    }
    printf("Announcement: %s\n", s);
};

/*
 * Checks for valid username
 *      not empty
 *      not already in use
 * Returns 0 if valid, else returns -1
 */
int check_username(struct client *active_clients, char* s) {
    // printf("check_username: (%s)\n", s);

    // check for empty string
    if (s[0] == '\0' || s == NULL) {
        printf("New User: empty strung.\n");
        return -1;
    }

    // find duplicates
    for ( struct client *curr = active_clients ; curr != NULL; curr = curr->next ) {
        // printf("check_username: against (%s)\n", curr->username);
        if (strcmp(s, curr->username) == 0) {
            printf("New User: duplicate name found.\n");
            return -1;
        }
    }
    // printf("check_username: (%s) no match!\n", s);
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
        if (buf[i] == '\r' && buf[i+1] == '\n'){
            return i + 2;
        }
    }
    return -1;
}

/*
 * Return the index of the end of the first word
 * Return -1 if not found
 */
int find_word(const char *s, int n){

    int i = 0;
    int max = max_cmd_length(); // restrict to cmd length

    for ( ; i < n && i < max ; i++ ) {
        // find space or end of line
        if (s[i] == ' ' || s[i] == '\0' || s[i] == '\n' || (s[i] == '\r' && s[i + 1] == '\n')){
            return i;
        }
    }
    return -1;
}

/*
 * Parse a string into a possible command
 * Return 0 if successful, otherwise -1
 */
int slice_command(const char *s, int n, char *command) {

    int slice_index = find_word(s, n);
        if (slice_index == -1) return -1;
    int slice_length = slice_index + 1; //include null terminator

    memmove(command, s, slice_length - 1);

    return 0;
}

/*
 * Return the max string length of possible commands
 */
int max_cmd_length(){
    int max = 0;
        if (strlen(SEND_MSG) > max)     max = strlen(SEND_MSG);
        if (strlen(SHOW_MSG) > max)     max = strlen(SHOW_MSG);
        if (strlen(FOLLOW_MSG) > max)   max = strlen(FOLLOW_MSG);
        if (strlen(UNFOLLOW_MSG) > max) max = strlen(UNFOLLOW_MSG);
        if (strlen(QUIT_MSG) > max)     max = strlen(QUIT_MSG);
    return max + 1;
}

/*
 * Clear a client's inbuf to prepare for the next read
 */
void clear_inbuf(struct client *p) {
    memset(p->inbuf, '\0', BUF_SIZE);
    p->in_ptr = p->inbuf;
}

/*
 * Read from a client's file descriptor
 * Return the number of bytes read
 */
int read_from(struct client *p) {

        int inbuf = 0;          // How many bytes currently in buffer?
        int room = BUF_SIZE;    // How many bytes remaining in buffer?
        int nbytes;

        nbytes = read(p->fd, p->in_ptr, room);
            
            printf("[%d] Read %d bytes.\n", p->fd, nbytes);

            inbuf += nbytes;
            p->in_ptr = p->inbuf + inbuf;
            room = sizeof(p->inbuf) - inbuf;
    
        return nbytes;

}


//! Command Functions ----------------------------------------------------------

/* 
 * Take a client that has completed a text entry, and parse into commands.
 */
int parse_command(struct client *p, struct client **active_clients) {

    // catch empty strings
    if (p->inbuf[0] == '\0' || p->inbuf == NULL){

        printf("%s: invalid command.", p->username);
        char *msg = "Invalid command.\r\n";

        _write(p->fd, msg, strlen(msg));
        clear_inbuf(p);
        return -1;
    }

    int n = strlen(p->inbuf) + 1;
    int max_cmd_len = max_cmd_length();

    char *command = _malloc(max_cmd_len);
        memset(command, '\0', max_cmd_len);
        if (slice_command(p->inbuf, n, command) == -1) return -1;

    //! QUIT ---------------------------------------
    if (strcmp(command, QUIT_MSG) == 0){
        remove_client(active_clients, p->fd);
        free(command);
        return 0;
    } 
    //! FOLLOW ---------------------------------------
    else if (strcmp(command, FOLLOW_MSG) == 0){

        if (p->inbuf[strlen(command)] != ' ') return -1;

        memmove(p->inbuf, p->inbuf + strlen(command) + 1, n - strlen(command));

        follow(p, p->inbuf, active_clients);
        
        free(command);
        clear_inbuf(p);
        return 1;
    }
    //! UNFOLLOW -------------------------------------
    else if (strcmp(command, UNFOLLOW_MSG) == 0){


        memmove(p->inbuf, p->inbuf + strlen(command) + 1, n - strlen(command));

        unfollow(p, p->inbuf, active_clients);

        free(command);
        clear_inbuf(p);
        return 2;
    }
    //! SHOW -----------------------------------------
    else if (strcmp(command, SHOW_MSG) == 0){
        show(p);

        free(command);
        clear_inbuf(p);
        return 3;
    }
    //! SEND MSG -------------------------------------
    else if (strcmp(command, SEND_MSG) == 0){

        memmove(p->inbuf, p->inbuf + strlen(command) + 1, n - strlen(command));
        p->inbuf[n - strlen(command)] = '\0';

        send_message(p);

        free(command);
        clear_inbuf(p);
        return 4;
    }
    //! INVALID --------------------------------------
    else {
        printf("%s: invalid command.\n", p->username);
        char *msg = "Invalid command.\r\n";
        _write(p->fd, msg, strlen(msg));

        free(command);
        clear_inbuf(p);
        return -1;
    }
}

//! Command Functions ----------------------------------------------------------

/* 
 * Create a follow relationship between user and target username
 * Add target username to user's following list
 * Add user to target username's follower list
 * iff both following/follower list is < FOLLOW_LIMIT
 */
void follow(struct client *p, char *target, struct client **active_clients){
    struct client *curr = *active_clients;
    int found = 0;
    int following_i = -1;
    int follower_i = -1;

    for ( ; curr != NULL; curr = curr->next ) {
        if (strcmp(target, curr->username) == 0) {
            found = 1;
            break;
        }
    }
    if (found == 1) {
        for (int i = 0; i < FOLLOW_LIMIT - 1; i++){
            if (p->following[i] == curr){
                printf("%s: Follow failed: already following %s \n", p->username, target);

                char *msg = "Follow failed: already following.\r\n";
                _write(p->fd, msg, strlen(msg));
                return;
            }
        }
        for (int i = 0; i < FOLLOW_LIMIT - 1; i++){
            if (following_i == -1 && p->following[i] == NULL) following_i = i;
            if (follower_i == -1 && curr->followers[i] == NULL) follower_i = i;
            if (following_i != -1 && follower_i != -1) break;
        }
        if (following_i == -1) {
            printf("%s: Follow failed: %s has reached their following limit.\n", p->username, p->username);
            char *msg = "Follow failed: max following reached.\r\n";
            _write(p->fd, msg, strlen(msg));
        }
        else if (follower_i == -1) {
            printf("%s: Follow failed: %s has reached their follower limit.\n", p->username, target);
            char *msg = "Follow failed: target max followers reached.\r\n";
            _write(p->fd, msg, strlen(msg));
        }
        else {
            p->following[following_i] = curr;
            curr->followers[follower_i] = p;
            printf("%s: followed %s.\n", p->username, target);
            char *msg = "Followed.\r\n";
            _write(p->fd, msg, strlen(msg));
        }

    } else {
        printf("%s: Follow failed: (%s) not found.\n", p->username, target);
        char *msg = "Follow failed: user not found.\r\n";
        _write(p->fd, msg, strlen(msg));
    }
    
}

/* 
 * Remove a follow relationship between user and target username
 * Remove target username to user's following list
 * Remove user to target username's follower list
 */
void unfollow(struct client *p, char *target, struct client **active_clients){
    struct client *curr = *active_clients;
    int found = 0;

    for ( ; curr != NULL; curr = curr->next ) {
        if (strcmp(target, curr->username) == 0) {
            found = 1;
            break;
        }
    }
    if (found == 1) {
        int found_follow = 0;
        for (int i = 0; i < FOLLOW_LIMIT - 1; i++){
            if ((p->following[i]) == curr){
                (p->following[i]) = NULL;
                found_follow = 1;
                break;
            }
        }

        if (found_follow == 0) {
            printf("%s: unfollow failed: was not following %s\n", p->username, target);
            char *msg = "Unfollow failed: not currently following.\r\n";
            _write(p->fd, msg, strlen(msg));
        }

        for (int i = 0; i < FOLLOW_LIMIT - 1; i++){
            if ((curr->followers[i]) == p){
                (curr->followers[i]) = NULL;
                break;
            }
        }

        printf("%s: unfollowed %s\n", p->username, target);
        char *msg = "Unfollowed.\r\n";
        _write(p->fd, msg, strlen(msg));

    } else {
        printf("%s: unfollow failed: no username (%s) found\n", p->username, target);
        char *msg = "Unfollow failed: username not found.\r\n";
        _write(p->fd, msg, strlen(msg));
    }
    
}

/* 
 * Send a message string to each follower of user
 */
void send_message(struct client *p) {
    int empty_msg_i = -1;

    for (int i = 0; i < MSG_LIMIT; i++){
        if(p->message[i][0] == '\0'){
            empty_msg_i = i;
            break;
        }
    }
    
    if (empty_msg_i == -1) {
        printf("%s: reached their message limit.\n", p->username);
        char *msg = "Send failed: message limit reached.\r\n";
        _write(p->fd, msg, strlen(msg));
        return;
    }

    if (p->inbuf[0] == '\0') {
        printf("%s: send invalid message\n", p->username);
        char *msg = "Send failed: invalid message.\r\n";
        _write(p->fd, msg, strlen(msg));
        return;
    }

    char *new_msg = p->message[empty_msg_i];
        strncat(new_msg, p->inbuf, MSG_LENGTH);
    //prepare message to send
    char send_msg[BUF_SIZE];
        memset(send_msg, '\0', BUF_SIZE);
        strncat(send_msg, p->username, strlen(p->username));
        strncat(send_msg, ": ", 2);
        strncat(send_msg, new_msg, MSG_LENGTH);
        strncat(send_msg, "\r\n", 2);
    //send to followers
    for (int i = 0; i < FOLLOW_LIMIT - 1; i++) {
        if (p->followers[i] != NULL){
            _write((p->followers[i])->fd, send_msg, strlen(send_msg));
        }
    }

    printf("%s: sent: %s\n", p->username, new_msg);
}

/* 
 * Receive every past message string from each client which the user follows
 */
void show(struct client *p) {
    struct client *curr;

    for (int i = 0; i < FOLLOW_LIMIT - 1; i++){
        if ((p->following[i]) != NULL) {
            curr = p->following[i];
            int j = 0;

            while (j < MSG_LIMIT) {

                if (curr->message[j][0] == '\0') break;

                //prepare message to send
                char send_msg[BUF_SIZE];
                    memset(send_msg, '\0', BUF_SIZE);
                    strncat(send_msg, p->username, strlen(p->username));
                    strncat(send_msg, " wrote: ", 8);
                    strncat(send_msg, curr->message[j], MSG_LENGTH);
                    strncat(send_msg, "\r\n", 2);

                //send to follower
                _write(p->fd, send_msg, strlen(send_msg));
                j++;
            }
            printf("%s: show messages from %s..\n", p->username, curr->username);
        }
    }
}

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

                        int read_num = read_from(p);

                        if (read_num == 0){
                            remove_client(&new_clients, cur_fd);
                            break;
                        }

                        int where;
                        if ((where = find_network_newline(p->inbuf, BUF_SIZE)) > 0){

                            p->inbuf[where - 2] = '\0';
                                printf("[%d] Found newline: %s\n", p->fd, p->inbuf);

                            if (check_username(active_clients, p->inbuf) != -1) {

                                memset(p->username, '\0', BUF_SIZE);
                                strncpy(p->username, p->inbuf, strlen(p->inbuf));

                                activate_client(p, &active_clients, &new_clients);

                                char msg[strlen(p->username) + strlen(JOINED_MSG)];
                                    memset(msg, '\0', strlen(p->username) + strlen(JOINED_MSG));
                                    strncpy(msg, p->username, strlen(p->username));
                                    char *joined = JOINED_MSG;
                                    strncat(msg, joined, strlen(joined));
                                announce(active_clients, msg);

                                clear_inbuf(p);
                                printf("%s: activated.\n", p->username);
                            }
                            else {
                                // prompt user to re-enter username
                                printf("[%d] Invalid username.", p->fd);
                                clear_inbuf(p);

                                char *invalid = INVUSR_MSG;
                                if (write(p->fd, invalid, strlen(invalid)) == -1) {
                                    fprintf(stderr, 
                                        "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                                    remove_client(&new_clients, clientfd);
                                }
                            }
                            handled = 1;
                            break;
                        }
                    }
                }
                
                if (!handled) {
                    // Check if this socket descriptor is an active client
                    for (p = active_clients; p != NULL; p = p->next) {
                        if (cur_fd == p->fd) {

                            int read_num = read_from(p);

                            if (read_num == 0){
                                remove_client(&active_clients, cur_fd);
                                break;
                            }

                            int where;
                            if ((where = find_network_newline(p->inbuf, BUF_SIZE)) != -1){
                                p->inbuf[where - 2] = '\0';
                                printf("[%d] Found newline: %s\n", p->fd, p->inbuf);
                                parse_command(p, &active_clients);
                            } 
                            break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
