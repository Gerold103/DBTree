#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#include "btree.h"

struct DB *dbcreate(const char *file, const struct DBC conf);
struct DB *dbopen  (const char *file);

int db_close (struct DB *db) { return 0; }
int db_del (struct DB *db, struct DBT *key) { return 0; }
int db_get (struct DB *db, struct DBT *key, struct DBT *data) { return 0; }
int db_put (struct DB *db, struct DBT *key, struct DBT *data);

void         bit_set(char val, int pos, char *bits);
int          bit_get(int pos, char *bits);
struct Node *alloc_node  (struct DB *db);
struct Node *load_node   (struct DB *db, int page);
int          dump_node   (struct DB *db, struct Node *node);
int          compare_keys(struct DBT *left, struct DBT *right);
int insert(struct DB *db, struct DBT *key, struct DBT *data, struct Node *node);
int          split_node_by_rec(struct DB *db, struct Node *node, int pos);

//----------------------------------------------------------------------------------------

struct DB *dbcreate(const char *file, const struct DBC conf)
{
    printf("dbcreate: start db creating\n");
    struct DB *db = (struct DB *)malloc(sizeof(struct DB));
    db->config = conf;
    db->db_file = fopen(file, "w+");
    db->capacity = conf.db_size / conf.chunk_size;

    db->get = db_get;
    db->close = db_close;
    db->del = db_del;
    db->put = db_put;

    db->first_free = ceil((db->capacity + 0.0) / conf.chunk_size) + 1;
    db->bitmask = (char *)calloc(db->capacity, 1);
    for (int i = 0; i < db->first_free; ++i) {
        bit_set(1, i, db->bitmask);
    }
    db->root = alloc_node(db);
    db->root->size = 0;
    db->root->tree_param = (conf.chunk_size - 32) / (2 * sizeof(struct Record));
    db->root->flags |= IS_LEAF;
    printf("dbcreate: end db creating\n");
    return db;
}

struct DB *dbopen(const char *file)
{
    printf("dbopen: start db opening\n");
    struct DB *db = (struct DB *)malloc(sizeof(struct DB));
    FILE *fd = fopen(file, "r+");
    fread(db, sizeof(struct DB), 1, fd);

    db->db_file = fd;
    db->close = db_close;
    db->del = db_del;
    db->get = db_get;
    db->put = db_put;
    db->bitmask = (char *)calloc(db->capacity, 1);
    fseek(fd, db->config.chunk_size, SEEK_SET);
    fread(db->bitmask, 1, db->capacity, fd);
    db->root = load_node(db, db->root_offset);
    printf("dbopen: end db opening\n");
    return db;
}

void bit_set(char val, int pos, char *bits)
{
    bits[pos / 8] = bits[pos / 8] & !(1 << (pos % 8)) || (val << (pos % 8));
}

int bit_get(int pos, char *bits)
{
    return (bits[pos / 8] >> (pos % 8)) & 1;
}

struct Node *alloc_node(struct DB *db)
{
    printf("alloc_node: start node allocating\n");
    struct Node *node = (struct Node *)calloc(1, db->config.chunk_size);
    while (bit_get(db->first_free, db->bitmask)) {
        db->first_free++;
    }
    bit_set(1, db->first_free, db->bitmask);
    node->offset = db->first_free;
    db->first_free++;
    printf("alloc_node: end node allocating\n");
    return node;
}

struct Node *load_node (struct DB *db, int page)
{
    printf("load_node: start node loading\n");
    struct Node *node = (struct Node *)calloc(1, db->config.chunk_size);
    fseek(db->db_file, page * db->config.chunk_size, SEEK_SET);
    fread(node, db->config.chunk_size, 1, db->db_file);
    printf("load_node: end node loading\n");
    return node;
}

int dump_node (struct DB *db, struct Node *node)
{
    printf("dump_node: start node dumping\n");
    fseek(db->db_file, node->offset * db->config.chunk_size, SEEK_SET);
    fwrite(node, db->config.chunk_size - 1, 1, db->db_file);
    printf("dump_node: end node dumping\n");
    return 0;
}

int compare_keys(struct DBT *left, struct DBT *right)
{
    printf("compare_keys: %s ; %s; ", left->data, right->data);
    int result = memcmp(left->data, right->data, fmin(left->size, right->size));
    if (!result) result = left->size - right->size;
    printf("result = %d\n", result);
    return result;
}

int insert(struct DB *db, struct DBT *key, struct DBT *data, struct Node *node)
{
   printf("insert: start inserting\n");
   int pos_for_ins = 0;
   //Finds position for new record
   while ((pos_for_ins < node->size) &&
          (compare_keys(&(node->records[pos_for_ins].key), key) < 0)) {
       ++pos_for_ins;
   }

   //If keys are equal, that it is neccessary to rewrite value
   if (!compare_keys(key, &(node->records[pos_for_ins].key))) {
       node->records[pos_for_ins].data = *data;
       printf("insert: end inserting. Record with such key already was in base\n");
       return dump_node(db, node);
   }

   if (node->flags & IS_LEAF) {
       node->records[node->size + 1].offset = node->records[node->size].offset;
       for (int j = node->size - 1; j >= pos_for_ins; --j) {
           node->records[j + 1] = node->records[j];
       }
       node->records[pos_for_ins].key = *key;
       node->records[pos_for_ins].data = *data;
       node->size++;
       printf("insert: end inserting in leaf node\n");
       return dump_node(db, node);
   }

   //If it is new record and we are not in leaf-node, but inside tree
   //Load the node-child of record on <pos_for_ins> position
   struct Node *chld = load_node(db, node->records[pos_for_ins].offset);

   //If child is full
   if (chld->size >= 2 * chld->tree_param - 1) {
       //splitting node by this position
       free(chld);
       if (split_node_by_rec(db, node, pos_for_ins)) {
           printf("insert: end inserting with error while spliting\n");
           return -1;
       }
       //move position if val on current is smaller
       if (compare_keys(&(node->records[pos_for_ins].key), key) < 0) {
           ++pos_for_ins;
       }
       //load this new node
       chld = load_node(db, node->records[pos_for_ins].offset);
   }
   //fall down in this neccessary child
   int result = insert(db, key, data, chld);
   free(chld);
   printf("insert: end inserting inside tree\n");
   return result;
}

int split_node_by_rec(struct DB *db, struct Node *node, int pos)
{
    printf("split_node_by_rec: start spliting\n");
    struct Node *new_child = alloc_node(db);
    struct Node *old_child = load_node(db, node->records[pos].offset);
    int tree_param = old_child->tree_param;
    new_child->tree_param = tree_param;
    new_child->flags = old_child->flags;
    new_child->size = old_child->size - tree_param;
    for (int i = 0; i < new_child->size; ++i) {
        new_child->records[i] = old_child->records[i + tree_param];
    }
    new_child->records[new_child->size].offset = old_child->records[old_child->size].offset;
    node->records[node->size + 1].offset = node->records[node->size].offset;
    for (int i = node->size; i > pos; --i) {
        node->records[i] = node->records[i - 1];
    }
    node->records[pos].offset = old_child->offset;
    node->records[pos + 1].offset = new_child->offset;
    node->records[pos].key = old_child->records[tree_param - 1].key;
    node->records[pos].data = old_child->records[tree_param - 1].data;
    old_child->size = tree_param - 1;
    node->size++;
    dump_node(db, new_child);
    dump_node(db, old_child);
    dump_node(db, node);
    free(new_child);
    free(old_child);
    printf("split_node_by_rec: end spliting\n");
    return 0;
}

int db_put (struct DB *db, struct DBT *key, struct DBT *data)
{
    printf("db_put: start putting\n");
    struct Node *root = db->root;
    if (root->size >= 2 * root->tree_param - 1) {
        struct Node *old_root = root;
        root = alloc_node(db);
        db->root = root;
        root->flags &= !IS_LEAF;
        root->size = 0;
        root->tree_param = old_root->tree_param;
        root->records[0].offset = old_root->offset;
        free(old_root);
        split_node_by_rec(db, root, 0);
    }
    return insert(db, key, data, root);
    printf("db_put: end putting\n");
}

void print_tree(struct DB *db, struct Node *node, int k)
{
    struct Node *ch;
    for (int i = 0; i < node->size; ++i) {
        if (!(node->flags & IS_LEAF)) {
            ch = load_node(db, node->records[i].offset);
            print_tree(db, ch, k + 1);
            free(ch);
        }
        for (int j = 0; j < k; ++j) printf("--");
        printf("[%s => %s]\n", node->records[i].key.data, node->records[i].data.data);
    }
    if (!(node->flags & IS_LEAF)) {
        ch = load_node(db, node->records[node->size].offset);
        print_tree(db, ch, k + 1);
        free(ch);
    }
}
