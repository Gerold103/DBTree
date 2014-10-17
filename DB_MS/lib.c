#define _GNU_SOURCE
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include "lib.h"
#include "bit.h"




//------------------------------------------------A U X I L I A R Y   F U N C T I O N S------------------------------------------------

void _init_pool(struct PoolPages *pool, int fd, size_t page_size, size_t db_size);

struct Node *_init_node(size_t parent_page, size_t flags, size_t size, size_t childs_size,
                        size_t *childs, struct DBT *keys, struct DBT *values);

bool _dump_new_node(struct DB *db, struct Node *node);

struct Node *_read_node_from_page(struct DB *db, size_t page);

/*
Create new DB
*/
struct DB *dbcreate(char *file, struct DBC config)
{
	struct DB *res = (struct DB *)malloc(sizeof(struct DB));
    res->p = (struct DBPrivate *)malloc(sizeof(struct DBPrivate));
    size_t db_size = config.db_size;
    size_t page_size = config.page_size;
    int fd = open(file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

    posix_fallocate(res->p->pool.fd, 0, config.db_size + sizeof(struct DBMeta));

    res->p->db_name = file;
    res->p->meta.db_size = db_size;
    res->p->meta.page_size = page_size;

    _init_pool(&(res->p->pool), fd, page_size, db_size);

    size_t bytes_for_bitmask = ceil(res->p->pool.sizes.pages_count / 8.0);
    pwrite(fd, &(res->p->meta), sizeof(struct DBMeta), 0);
    pwrite(fd, res->p->pool.bitmask, bytes_for_bitmask, sizeof(struct DBMeta));

    res->p->top = NULL;
	return res;
}

struct DB *dbopen(char *file)
{
    struct DB *res = (struct DB *)malloc(sizeof(struct DB));
    res->p = (struct DBPrivate *)malloc(sizeof(struct DBPrivate));
    int fd = open(file, O_RDWR, 0);
    pread(fd, &(res->p->meta), sizeof(struct DBMeta), 0);
    size_t db_size = res->p->meta.db_size;
    size_t page_size = res->p->meta.page_size;
    _init_pool(&(res->p->pool), fd, page_size, db_size);

    size_t npages = res->p->pool.sizes.pages_count;
    size_t bytes_for_bitmask = ceil(npages / 8.0);
    size_t pages_for_bitmask = res->p->pool.sizes.pages_for_bitmask;

    pread(fd, res->p->pool.bitmask, bytes_for_bitmask, sizeof(struct DBMeta));

    size_t nodes = 0;
    size_t bitmask_start = pages_for_bitmask;
    size_t bitmask_end = npages;

    for (size_t i = bitmask_start; i < bitmask_end; ++i) {
        if (bit_test(res->p->pool.bitmask, i)) ++nodes;
    }

    if (nodes == 0) {
        res->p->top = NULL;
    } else {
        res->p->top = _read_node_from_page(res, res->p->pool.sizes.top_page);
    }
    res->p->pool.sizes.nodes = nodes;
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

//------------------------------------------------A U X I L I A R Y   F U N C T I O N S------------------------------------------------

void _init_pool(struct PoolPages *pool, int fd, size_t page_size, size_t db_size)
{
    size_t pages_count = db_size / page_size;
    size_t bytes_for_bitmask = ceil(pages_count / 8.0);
    size_t pages_for_bitmask = ceil((bytes_for_bitmask + 0.0) / page_size);
    size_t node_meta_size = sizeof(size_t) * 9;
    size_t one_pair_size = ceil((page_size - node_meta_size + 0.0) / K);
    size_t top_page = pages_for_bitmask;

    void *bitmask = (void *)malloc(bytes_for_bitmask);
    memset(bitmask, 0, bytes_for_bitmask);
    for (int i = 0; i < pages_for_bitmask; ++i) {
        bit_set(bitmask, i);
    }

    pool->fd = fd;
    pool->bitmask = bitmask;
    pool->sizes.node_meta_size = node_meta_size;
    pool->sizes.one_pair_size = one_pair_size;
    pool->sizes.pages_count = pages_count;
    pool->sizes.pages_for_bitmask = pages_for_bitmask;
    pool->sizes.top_page = top_page;
    pool->sizes.nodes = 0;
}

struct Node *_init_node(size_t parent_page, size_t flags, size_t size, size_t childs_size,
                        size_t *childs, struct DBT *keys, struct DBT *values)
{
    struct Node *node = (struct Node *)malloc(sizeof(struct Node));
    memset(node, 0, sizeof(struct Node));

    node->meta.page = 0;
    node->meta.parent_page = parent_page;
    node->meta.flags = flags;
    node->meta.size = size;
    for (int i = 0; i < childs_size; ++i) {
        node->childs[i] = childs[i];
    }
    for (int i = childs_size; i < K + 1; ++i) {
        node->childs[i] = 0;
    }
    for (int i = 0; i < size; ++i) {
        node->keys[i] = keys[i];
        node->values[i] = values[i];
    }
    for (int i = size; i < K; ++i) {
        node->keys[i].bytes = NULL;
        node->keys[i].size = 0;
        node->values[i].bytes = NULL;
        node->values[i].size = 0;
    }
    return node;
}

bool _dump_new_node(struct DB *db, struct Node *node)
{
    size_t bitmask_start = db->p->pool.sizes.pages_for_bitmask;
    size_t bitmask_end = db->p->pool.sizes.pages_count;
    size_t i = bitmask_start;
    for (; i < bitmask_end; ++i) {
        if (!bit_test(db->p->pool.bitmask, i)) break;
    }

    if (i == bitmask_end) return false;

    node->meta.page = i;
    size_t page_offset = sizeof(struct DBMeta) + i * (db->p->meta.page_size);
    pwrite(db->p->pool.fd, &(node->meta), sizeof(struct NodeMeta), page_offset);
    page_offset += sizeof(struct NodeMeta);

    pwrite(db->p->pool.fd, node->childs, sizeof(size_t) * (K + 1), page_offset);
    page_offset += sizeof(size_t) * (K + 1);
    pwrite(db->p->pool.fd, node->keys, sizeof(struct DBT) * K, page_offset);
    page_offset += sizeof(struct DBT) * K;
    pwrite(db->p->pool.fd, node->values, sizeof(struct DBT) * K, page_offset);
    return true;
}

struct Node *_read_node_from_page(struct DB *db, size_t page)
{
    size_t bitmask_start = db->p->pool.sizes.pages_for_bitmask;
    size_t bitmask_end = db->p->pool.sizes.pages_count;
    if ((page < bitmask_start) || (page >= bitmask_end) || (!bit_test(db->p->pool.bitmask, page))) {
        return NULL;
    }

    struct Node *node = (struct Node *)malloc(sizeof(struct Node));
    node->meta.page = page;
    size_t page_offset = sizeof(struct DBMeta) + page * (db->p->meta.page_size);
    pread(db->p->pool.fd, &(node->meta), sizeof(struct NodeMeta), page_offset);
    page_offset += sizeof(struct NodeMeta);

    pread(db->p->pool.fd, node->childs, sizeof(size_t) * (K + 1), page_offset);
    page_offset += sizeof(size_t) * (K + 1);
    pread(db->p->pool.fd, node->keys, sizeof(struct DBT) * K, page_offset);
    page_offset += sizeof(struct DBT) * K;
    pread(db->p->pool.fd, node->values, sizeof(struct DBT) * K, page_offset);
    return node;
}

void printDB(struct DB *db)
{
    printf("Name: %s\n", db->p->db_name);
    if (db->p->top == NULL) {
        printf("Top: NULL\n");
    } else {
        printf("Top: page = %d, parent page = %d, size = %d\n", (int)(db->p->top->meta.page), (int)(db->p->top->meta.parent_page),
               (int)(db->p->top->meta.size));
        printf("childs: ");
        for (int i = 0; i < K + 1; ++i) {
            printf("%d ", (int)(db->p->top->childs[i]));
        }
        printf("\n");
    }
    printf("Page pool: ");
    struct PoolPages *pool = &(db->p->pool);
    printf("pages count = %d, pages for bitmask = %d, node meta size = %d, one pair size = %d, top page = %d, nodes = %d\n",
           (int)(pool->sizes.pages_count), (int)(pool->sizes.pages_for_bitmask), (int)(pool->sizes.node_meta_size),
           (int)(pool->sizes.one_pair_size), (int)(pool->sizes.top_page), (int)(pool->sizes.nodes));
    printf("DB meta: db size = %d, page size = %d\n", (int)(db->p->meta.db_size), (int)(db->p->meta.page_size));
}
