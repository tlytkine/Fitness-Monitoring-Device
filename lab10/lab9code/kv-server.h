#ifndef _KV_SERVER_H
#define _KV_SERVER_H

#define BUCKET_COUNT 8
#define SOCKET_BACKLOG 8
#define BUF_SIZE 128
#define KEY_SIZE 32
#define VAL_SIZE 32

#define ERROR(lbl, err) \
    do { \
        perror(err); \
        goto lbl; \
} while (0)

/* A definition for a node in the hash table
 * These structs are set up to be used in a doubly linked list
 */
struct ht_node {
    char key[KEY_SIZE];
    char value[VAL_SIZE];
    struct ht_node *next;
    struct ht_node *prev;
};

#endif
