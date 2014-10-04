#ifndef _LIB_H
#define _LIB_H

//----------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

//-----------------------------------------------------D A T A   B A S E   T Y P E----------------------------------------------------

struct DBT {
     void  *data;
     size_t size;
};

//----------------------------------------------------------D A T A   B A S E----------------------------------------------------------

struct __DB_private__;

struct DB {
    /* Public API */
    int (*close)(struct DB *db);
    int (*del)(const struct DB *db, const struct DBT *key);
    int (*get)(const struct DB *db, struct DBT *key, struct DBT *data);
    int (*put)(const struct DB *db, struct DBT *key, const struct DBT *data);
    size_t (*getMaxSize)(const struct DB *db);
    size_t (*getCurrentSize)(const struct DB *db);
    size_t (*getChunkSize)(const struct DB *db);
    size_t (*getMemSize)(const struct DB *db);
    //int (*sync)(const struct DB *db);
	/* Private API */
	struct __DB_private__ *private;
}; /* Need for supporting multiple backends (HASH/BTREE) */

//---------------------------------------------------D A T A   B A S E   C O N F I G---------------------------------------------------

struct DBC {
        /* Maximum on-disk file size */
        /* 512MB by default */
        size_t db_size;
        /* Maximum chunk (node/data chunk) size */
        /* 4KB by default */
        size_t chunk_size;
        /* Maximum memory size */
        /* 16MB by default */
        size_t mem_size;
};

struct DB *dbcreate(const char *file, const struct DBC *conf);
struct DB *dbopen  (const char *file); /* Metadata in file */

#endif