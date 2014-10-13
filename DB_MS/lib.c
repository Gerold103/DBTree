#define _GNU_SOURCE
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include "lib.h"
#include "bit.h"
//----------------------------------------------------------D A T A   B A S E----------------------------------------------------------

struct DBMeta {
    uint32_t page_size;
    uint32_t db_size;
};

struct DBPrivate {
    char *db_name;
    int fd;
    size_t pages_count;
    size_t pages_for_bitmask;
    void *bitmask;
    struct DBMeta meta;
    size_t top_page;
};

struct DBT {

};

//---------------------------------------------------------

/*
Create new DB
*/
struct DB *dbcreate(char *file, struct DBC config)
{
	struct DB *res = (struct DB *)malloc(sizeof(struct DB));
    res->p = (struct DBPrivate *)malloc(sizeof(struct DBPrivate));
    res->p->meta.db_size = config.db_size;
    res->p->meta.page_size = config.page_size;
    res->p->fd = open(file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    posix_fallocate(res->p->fd, 0, config.db_size);

    write(res->p->fd, &(res->p->meta), sizeof(struct DBMeta));

    size_t npages = config.db_size / config.page_size;
    size_t bytes_for_bitmask = ceil(npages / 8.0);
    size_t pages_for_bitmask = ceil((bytes_for_bitmask + 0.0) / config.page_size);
    void *bitmask = (void *)malloc(bytes_for_bitmask);
    memset(bitmask, 0, bytes_for_bitmask);
    memset(bitmask, 1, pages_for_bitmask);

    write(res->p->fd, bitmask, bytes_for_bitmask);

    res->p->bitmask = bitmask;
    res->p->pages_count = npages;
    res->p->pages_for_bitmask = pages_for_bitmask;
    res->p->top_page = res->p->pages_for_bitmask;
	return res;
}

struct DB *dbopen(char *file)
{
    struct DB *res = (struct DB *)malloc(sizeof(struct DB));
    res->p = (struct DBPrivate *)malloc(sizeof(struct DBPrivate));
    int fd = open(file, O_RDWR, 0);

    read(fd, &(res->p->meta), sizeof(struct DBMeta));

    size_t npages = res->p->meta.db_size / res->p->meta.page_size;
    size_t bytes_for_bitmask = ceil(npages / 8.0);
    size_t pages_for_bitmask = ceil((bytes_for_bitmask + 0.0) / res->p->meta.page_size);
    void *bitmask = (void *)malloc(bytes_for_bitmask);
    memset(bitmask, 0, bytes_for_bitmask);
    memset(bitmask, 1, pages_for_bitmask);

    read(fd, bitmask, bytes_for_bitmask);

    res->p->bitmask = bitmask;
    res->p->pages_count = npages;
    res->p->pages_for_bitmask = pages_for_bitmask;
    res->p->top_page = res->p->pages_for_bitmask;
    res->p->fd = fd;
    res->p->db_name = file;
    return res;
}

/*
Close existing DB
*/
void db_close(struct DB *db) {
	return;
}

/*
Delete existing DB
*/
int db_del(struct DB *db, void *key, size_t key_len) {
	return 0;
}

/*
Get value from DB by key
*/
int db_get(struct DB *db, void *key, size_t key_len,
	   void **val, size_t *val_len) {
	return 0;
}

/*
Put new pair-key, value into DB
*/
int db_put(struct DB *db, void *key, size_t key_len,
	   void *val, size_t val_len) {
	return 0;
}
