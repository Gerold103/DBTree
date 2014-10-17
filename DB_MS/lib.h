#ifndef _LIB_H
#define _LIB_H

#define K 4

//----------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct DBT;

struct DBPrivate;

enum Flags {
    IS_LEAF = 0x01,
    IS_TOP  = 0x02
};

//----------------------------------------------------------D A T A   B A S E----------------------------------------------------------

struct DBMeta {
    uint32_t page_size;
    uint32_t db_size;
};

struct DBSizes {
    size_t pages_count;
    size_t pages_for_bitmask;
    size_t node_meta_size;
    size_t one_pair_size;
    size_t top_page;
    size_t nodes;
};

struct PoolPages {
    void *bitmask;
    struct DBSizes sizes;
    int fd;
};

struct DBT {
    void *bytes;
    size_t size;
};

struct NodeMeta {
    size_t page;
    size_t parent_page;
    size_t flags;
    size_t size;
};

struct Node {
    struct NodeMeta meta;
    size_t childs[K + 1]; //child pages
    struct DBT keys[K];
    struct DBT values[K];
};

struct DBPrivate {
    char *db_name;
    struct Node *top;
    struct PoolPages pool;
    struct DBMeta meta;
};


struct DB {
    struct DBPrivate *p;
};

//---------------------------------------------------D A T A   B A S E   C O N F I G---------------------------------------------------

struct DBC {
        /* Maximum on-disk file size */
        /* 512MB by default */
        size_t db_size;
        /* Maximum chunk (node/data chunk) size */
        /* 4KB by default */
        size_t page_size;
        /* Maximum memory size */
        /* 16MB by default */
        size_t mem_size;
};

struct DB *dbcreate(char *file, struct DBC config);
struct DB *dbopen(char *file);
void printDB(struct DB *db);

/*void db_close(struct DB *db);
int  db_del  (struct DB *db, void *key, size_t key_len);
int  db_get  (struct DB *db, void *key, size_t key_len, void **val, size_t *val_len);
int  db_put  (struct DB *db, void *key, size_t key_len, void *val, size_t val_len);*/

#endif
