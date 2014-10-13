#ifndef _LIB_H
#define _LIB_H
#define DB_KEYS_PER_NODE 3

//----------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct DBT;

struct DBPrivate;

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
/*void db_close(struct DB *db);
int  db_del  (struct DB *db, void *key, size_t key_len);
int  db_get  (struct DB *db, void *key, size_t key_len, void **val, size_t *val_len);
int  db_put  (struct DB *db, void *key, size_t key_len, void *val, size_t val_len);*/

#endif
