/************************************************
 *              CSCI 3410, Lab 9
 *          Mini KV Store (solution)
 *
 * Copyright 2017 Phil Lopreiato
 *
 * With help from:
 *  - http://beej.us/guide/bgnet/
 *  - https://github.com/gwAdvNet2015/gw-kv-store
 *
 * This program is licensed under the MIT license.
 *************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "kv-server.h"

/* The array of pointers that will constitute our hash table */
struct ht_node **ht;

/* Local function prototypes */
int init_local_socket(char *port);
int hash_key(char * key);
#if 1
void handle_request(char *request, char **response);
void handle_get(char *key, char **response);
void handle_set(char *key, char *value, char **response);
void handle_delete(char *key, char **response);
void handle_create(char *key, char **response);
#endif 
struct ht_node *search_bucket(struct ht_node *start, char *key);

int
main(int argc, char **argv) {
    char *port;
    int srvfd;
    int clientfd;
    char msg[BUF_SIZE];
    char *resp;
    int ret = 0;
    struct sockaddr *clientaddr = NULL;
    socklen_t *clientaddrlen = NULL;

    if (argc != 2) {
        fprintf(stderr, "Bad arguments. Pass the port as a command line parameter\n");
        return -1;
    }

    // Parse command line param to get port
    port = argv[1];

    // Init hashtable (array of pointers) and response buffer
    ht = calloc(sizeof(struct ht_node *), BUCKET_COUNT);
    resp = calloc(sizeof(char), BUF_SIZE);

    // Create socket and start listening
    srvfd = init_local_socket(port);
    if (srvfd == -1) return -1;

    // Main loop, continue forever accepting clients and processing requests
    printf("Connected to localhost:%s\n", port);
    while (1) {
        clientfd = accept(srvfd,clientaddr,clientaddrlen);
        if (clientfd == -1) {
            perror("accept");
            continue;
        }

        // Read all that this client wants
        printf("New client\n");
        while (1) {
            memset(msg, 0, BUF_SIZE);
            ret = read(clientfd,(void*)msg,BUF_SIZE);
            if (ret < 0) {
                ERROR(srv_error, "read");
            } else if(ret == 0) {
                printf("Disconnect\n");
                break;
            } else {
                // Process the request.
                // This changes message to be the return string
             handle_request(msg, &resp);

                // Write the response back to the client
               int s = strlen(resp);
               ret = write(clientfd,(void*)resp,s);
                if (ret < 0) ERROR(srv_error, "write");
            }
        }

srv_error:
        close(clientfd);
    }

    free(ht);
    free(resp);
}
#if 1 
void
handle_request(char *msg, char **response) {
    char *cmd;
    char *arg1;
    char *arg2;

    // Parse out the arguments
    cmd = strtok(msg, " \n\r");
    arg1 = strtok(NULL, " \n\r");
    arg2 = strtok(NULL, " \n\r");

    if (strncmp("GET", msg, 3) == 0) {
        handle_get(arg1, response);
    } else if (strncmp("SET", msg, 3) == 0) {
        handle_set(arg1, arg2, response);
    } else if (strncmp("DELETE", msg, 6) == 0) {
        handle_delete(arg1, response);
    } else if (strncmp("CREATE", msg, 6) == 0) {
        handle_create(arg1, response);
    } else {
        sprintf(*response, "?\n");
    }
}

void
handle_get(char *key, char **response) {
    struct ht_node *node;
    int hash;

    if (key == NULL) {
        sprintf(*response, "NO KEY\n");
        return;
    }

    hash = hash_key(key);
    node = search_bucket(ht[hash], key);
    if (node) {
        sprintf(*response, "%s\n",node->value);
    } else {
        sprintf(*response, "NOT FOUND\n");
    }
}

void
handle_set(char *key, char *value, char **response) {
    struct ht_node *node;
    int hash;

    if (key == NULL) {
        sprintf(*response, "NO KEY\n");
        return;
    }

    if (value == NULL) {
        sprintf(*response, "NO VALUE\n");
        return;
    }

    hash = hash_key(key);
    node = search_bucket(ht[hash], key);
    if (node) {
        // TODO set the value of the node
        strcpy(node->value,value);
        sprintf(*response, "ok\n");
    } else {
        sprintf(*response, "NOT FOUND\n");
    }
}

void
handle_create(char *key, char **response) {
    struct ht_node *node;
    int hash;

    if (key == NULL) {
        sprintf(*response, "NO KEY\n");
        return;
    }

    hash = hash_key(key);
    node = search_bucket(ht[hash], key);
    if (!node) {
        // TODO: insert the node into the hash table
            int x = hash;
            int i;

            for(i = x; i < BUCKET_COUNT; i++){
                node = search_bucket(ht[i],key);
                if(node){
                    break;
                }
            }

        sprintf(*response, "ok\n");
        printf("Created new node %s\n", key);
    } else {
        sprintf(*response, "ALREADY EXISTS\n");
    }
}

void
handle_delete(char *key, char **response) {
    struct ht_node *node;
    int hash;

    if (key == NULL) {
        sprintf(*response, "NO KEY\n");
        return;
    }

    hash = hash_key(key);
    node = search_bucket(ht[hash], key);
    if (node && node == ht[hash] && ht[hash]->next == NULL) {
        // Node is the only this in this bucket
        // TODO delete the node from the list
        sprintf(*response, "ok\n");
        printf("Delete node %s\n", key);
    } else if (node && node == ht[hash]) {
        // Node is the head of a list
        // TODO delete the node from the list
        sprintf(*response, "ok\n");
        printf("Delete node %s\n", key);
    } else if (node) {
        // Node is somewhere in a list
        // TODO delete the node from the list
        sprintf(*response, "ok\n");
        printf("Delete node %s\n", key);
    } else {
        sprintf(*response, "NOT FOUND\n");
    }
}
#endif

struct ht_node *
search_bucket(struct ht_node *start, char *key) {
    // TODO Find the desired key in the given bucket
    struct ht_node *temp = start;
    while(temp->key!=key){
        temp = temp->next;
    }

    return temp;
}

int hash_key(char *key) {
    // TODO Implement some hash function on the given key

    int s = strlen(key);
    int i;
    // hash value 
    int x = 0;
    for(i = 0; i < s; i++){
        x = x * 10;
        x+= key[i] - '0';
    }
    x = x % BUCKET_COUNT;

    return x;
}

int
init_local_socket(char *port) {
    struct addrinfo hints, *server;
    int fd;
    int ret;
    int enable = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    /*
     * Look up info about the connection we want. Pass NULL to specify we're
     * looking at localhost. Hints helps the function guess what we want
     */
    ret = getaddrinfo(NULL, port, &hints, &server);
    if (ret != 0) {
        perror(gai_strerror(ret));
        return -1;
    }

    /* Create a new socket and track the file descriptor */
    fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (fd == -1) return -1;

    /* Set SO_REUSEADDR so we don't get "Address already in use" */
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (ret == -1) ERROR(socket_error, "setsockopt");

    /* Bind to the socket */
    ret = bind(fd, server->ai_addr, server->ai_addrlen);
    if (ret == -1) ERROR(socket_error, "bind");

    free(server);

    /* Now, start listening for incoming clients */
    listen(fd, SOCKET_BACKLOG);
    return fd;

socket_error:
    close(fd);
    return -1;
}
