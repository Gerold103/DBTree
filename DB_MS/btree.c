#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#include "btree.h"

struct DB *dbcreate(const char *file, const struct DBC conf);
struct DB *dbopen  (const char *file);

int db_close (struct DB *db);
int db_del (struct DB *db, struct DBT *key);
int db_get (struct DB *db, struct DBT *key, struct DBT *data);
int db_put (struct DB *db, struct DBT *key, struct DBT *data);

static int db_del_recurs(struct DB *db, struct Node *node, struct DBT *key);

void         bit_set            (char val, int pos, char *bits);
int          bit_get            (int pos, char *bits);
struct Node *alloc_node         (struct DB *db);
struct Node *load_node          (struct DB *db, int page);
int          delete_node        (struct DB *db, struct Node *node);
int          dump_node          (struct DB *db, struct Node *node);
int          compare_keys       (struct DBT *left, struct DBT *right);
int          insert             (struct DB *db, struct DBT *key, struct DBT *data, struct Node *node);
int          get                (struct DB *db, struct DBT *key, struct DBT *data, struct Node *node);
int          split_node_by_rec  (struct DB *db, struct Node *node, int pos);
int          find_free_after_key(struct Node *node, struct DBT *key);
void rewrite_record(struct Record *rec1, struct Record *rec2);
void rewrite_raw_record(struct DBT *key, struct DBT *data, struct Record *rec);
void rewrite_record_from_raw(struct Record *rec, struct DBT *key, struct DBT *data);
void move_records(int start, int end, int step, struct Record *records_left, struct Record *records_right);

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

    db->close(db);
    db = dbopen(file);
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

int delete_node(struct DB *db, struct Node *node)
{
    printf("delete_node: start node deleting\n");
    bit_set(0, node->offset, db->bitmask);
    db->first_free = fmin(db->first_free, node->offset);
    printf("delete_node: end node deleting\n");
    return 0;
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
   //Finds position for new record
   int i = find_free_after_key(node, key);

   //If keys are equal, that it is neccessary to rewrite value
   if (!compare_keys(key, &(node->records[i].key))) {
       node->records[i].data = *data;
       printf("insert: end inserting. Record with such key already was in base\n");
       return dump_node(db, node);
   }

   if (node->flags & IS_LEAF) {
       node->records[node->size + 1].offset = node->records[node->size].offset;
       move_records(node->size, i, -1, node->records, node->records);
       rewrite_record_from_raw(&(node->records[i]), key, data);
       node->size++;
       printf("insert: end inserting in leaf node\n");
       return dump_node(db, node);
   }

   //If it is new record and we are not in leaf-node, but inside tree
   //Load the node-child of record on <i> position
   struct Node *chld = load_node(db, node->records[i].offset);

   //If child is full
   if (chld->size >= 2 * chld->tree_param - 1) {
       //splitting node by this position
       free(chld);
       if (split_node_by_rec(db, node, i)) {
           printf("insert: end inserting with error while spliting\n");
           return -1;
       }
       //move position if val on current is smaller
       if (compare_keys(&(node->records[i].key), key) < 0) {
           ++i;
       }
       //load this new node
       chld = load_node(db, node->records[i].offset);
   }
   //fall down in this neccessary child
   int result = insert(db, key, data, chld);
   free(chld);
   printf("insert: end inserting inside tree\n");
   return result;
}

int get(struct DB *db, struct DBT *key, struct DBT *data, struct Node *node)
{
    printf("get: start getting\n");
    int i = find_free_after_key(node, key);
    if ((i < node->size) && (compare_keys(&(node->records[i].key), key) == 0)) {
        *data = node->records[i].data;
        printf("get: end getting, val is found\n");
        return 0;
    }
    if (!(node->flags & IS_LEAF)) {
        struct Node *next = load_node(db, node->records[i].offset);
        int res = get(db, key, data, next);
        free(next);
        return res;
    }
    printf("get: end getting, val isn't found\n");
    return -1;
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
    move_records(0, new_child->size, tree_param, new_child->records, old_child->records);
    new_child->records[new_child->size].offset = old_child->records[old_child->size].offset;
    node->records[node->size + 1].offset = node->records[node->size].offset;
    move_records(node->size, pos, -1, node->records, node->records);
    node->records[pos].offset = old_child->offset;
    node->records[pos + 1].offset = new_child->offset;
    rewrite_record(&(node->records[pos]), &(node->records[tree_param - 1]));
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

int find_free_after_key(struct Node *node, struct DBT *key)
{
    int i = 0;
    while ((i < node->size) && (compare_keys(&(node->records[i].key), key) < 0)) {
        ++i;
    }
    return i;
}

int db_close (struct DB *db)
{
    printf("db_close: start database closing\n");

    dump_node(db, db->root);
    db->root_offset = db->root->offset;
    fseek(db->db_file, 0, SEEK_SET);
    fwrite(db, sizeof(struct DB), 1, db->db_file);
    fseek(db->db_file, db->config.chunk_size, SEEK_SET);
    fwrite(db->bitmask, 1, db->capacity, db->db_file);
    free(db->bitmask);
    free(db->root);
    fclose(db->db_file);
    free(db);

    printf("db_close: end database closing\n");
    return 0;
}

int db_del (struct DB *db, struct DBT *key)
{
    printf("db_del: start value deleting\n");
    int result = db_del_recurs(db, db->root, key);
    if (db->root->size == 0) {
        struct Node *empty_node = load_node(db, db->root->records[0].offset);
        delete_node(db, db->root);
        free(db->root);
        db->root = empty_node;
    }
    printf("db_del: end value deleting\n");
    return result;
}

int db_get (struct DB *db, struct DBT *key, struct DBT *data)
{
    return get(db, key, data, db->root);
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

void lift_right_record(struct DB *db, struct Node *node, struct DBT *key, struct DBT *data)
{
    if (node->flags & IS_LEAF) {
        rewrite_raw_record(key, data, &(node->records[node->size - 1]));
    } else {
        struct Node *next = load_node(db, node->records[node->size].offset);
        lift_right_record(db, next, key, data);
        free(next);
    }
}

void lift_left_record(struct DB *db, struct Node *node, struct DBT *key, struct DBT *data)
{
    if (node->flags & IS_LEAF) {
        rewrite_raw_record(key, data, &(node->records[0]));
    } else {
        struct Node *next = load_node(db, node->records[0].offset);
        lift_left_record(db, node, key, data);
        free(next);
    }
}

int del_from_subtree(struct DB *db, struct Node *node, struct Node *child, void (*lifter)(struct DB *, struct Node *, struct DBT *, struct DBT *), int i)
{
   if (child->size < child->tree_param) return -1;
   lifter(db, child, &(node->records[i].key), &(node->records[i].data));
   db_del_recurs(db, child, &(node->records[i].key));
   return dump_node(db, node);
}

int db_del_recurs(struct DB *db, struct Node *node, struct DBT *key)
{
    int i = find_free_after_key(node, key);
    //If key is found
    if ((i < node->size) && (compare_keys(&(node->records[i].key), key) == 0)) {
        //Bottom of tree
        if (node->flags & IS_LEAF) {
            //Moving right records to left
            move_records(i, node->size - 1, 1, node->records, node->records);
            node->size--;
            return dump_node(db, node);
        } else {
            //Reorganize left subtree
            struct Node *left_child = load_node(db, node->records[i].offset);
            if (!del_from_subtree(db, node, left_child, lift_right_record, i)) {
                free(left_child);
                return 0;
            }
            //Reorganize right subtree
            struct Node *right_child = load_node(db, node->records[i + 1].offset);
            if (!del_from_subtree(db, node, right_child, lift_left_record, i)) {
                free(left_child);
                free(right_child);
                return 0;
            }
            //Left and right contains minimum records
            int ls = left_child->size;
            rewrite_record(&(left_child->records[ls]), &(node->records[i]));
            for (int k = i; k < node->size - 1; ++k) {
                rewrite_record(&(node->records[k]), &(node->records[k + 1]));
                node->records[k + 1].offset = node->records[k + 2].offset;
            }
            node->size--;
            //Merging right to left subtrees
            move_records(ls + 1, right_child->size + ls + 1, -1 - ls, left_child->records, right_child->records);
            left_child->size += right_child->size + 1;
            ls = left_child->size;
            int rs = right_child->size;
            left_child->records[ls].offset = right_child->records[rs].offset;
            delete_node(db, right_child);
            dump_node(db, node);
            int result = db_del_recurs(db, left_child, key);
            free(left_child);
            return result;
        }
    //Key is not found in current node
    } else {
        struct Node *next = load_node(db, node->records[i].offset);
        //If contains minimum records
        if (next->size < next->tree_param) {
            struct Node *left = NULL, *right = NULL;
            //If not first record
            if (i > 0) {
                left = load_node(db, node->records[i - 1].offset);
                //If contains more than minimum records
                if (left->size >= left->tree_param) {
                    next->records[next->size + 1].offset = next->records[next->size].offset;
                    move_records(next->size, 0, -1, next->records, next->records);
                    next->size++;
                    rewrite_record(&(next->records[0]), &(node->records[i - 1]));
                    rewrite_record(&(node->records[i - 1]), &(left->records[left->size - 1]));
                    next->records[0].offset = left->records[left->size].offset;
                    left->size--;
                    dump_node(db, left);
                    dump_node(db, node);
                    free(left);
                    int result = db_del_recurs(db, next, key);
                    free(next);
                    return result;
                }
            }
            //If not last record
            if (i < node->size) {
                right = load_node(db, node->records[i + 1].offset);
                //If contains more than minimum records
                if (right->size >= right->tree_param) {
                    rewrite_record(&(next->records[next->size]), &(node->records[i]));
                    next->records[next->size + 1].offset = right->records[0].offset;
                    next->size++;
                    rewrite_record(&(node->records[i]), &(right->records[0]));
                    move_records(0, right->size - 1, 1, right->records, right->records);
                    right->records[right->size - 1].offset = right->records[right->size].offset;
                    right->size--;
                    dump_node(db, right);
                    dump_node(db, node);
                    free(left);
                    free(right);
                    int result = db_del_recurs(db, next, key);
                    return result;
                }
            }
            if (i > 0) {
                int ls = left->size;
                rewrite_record(&(left->records[ls]), &(node->records[i - 1]));
                move_records(ls + 1, next->size + ls + 1, -1 - ls, left->records, next->records);
                left->size += next->size + 1;
                ls = left->size;
                left->records[ls].offset = next->records[next->size].offset;
                delete_node(db, next);
                free(next);
                for (int k = i - 1; k < node->size; ++k) {
                    rewrite_record(&(node->records[k]), &(node->records[k + 1]));
                    node->records[k + 1].offset = node->records[k + 2].offset;
                }
                node->size--;
                dump_node(db, node);
                dump_node(left, db);
                free(right);
                int result = db_del_recurs(db, left, key);
                free(left);
                return result;
            }
            if (i < node->size) {
                int ns = next->size;
                rewrite_record(&(next->records[ns]), &(node->records[i]));
                move_records(ns + 1, right->size + ns + 1, -1 - ns, next->records, right->records);
                next->size += right->size + 1;
                ns = next->size;
                next->records[ns].offset = right->records[right->size].offset;
                delete_node(db, right);
                free(right);
                for (int k = i; k < node->size - 1; ++k) {
                    rewrite_record(&(node->records[k]), &(node->records[k + 1]));
                    node->records[k + 1].offset = node->records[k + 2].offset;
                }
                node->size--;
                dump_node(db, node);
                dump_node(db, next);
                free(left);
                int result = db_del_recurs(db, next, key);
                free(next);
                return result;
            }
            return -1;
        } else {
            if (!(node->flags & IS_LEAF)) {
                int res = db_del_recurs(db, next, key);
                free(next);
                return res;
            } else {
                return 0;
            }
        }
    }
}

void rewrite_record(struct Record *rec1, struct Record *rec2)
{
    rec1->data = rec2->data;
    rec1->key = rec2->key;
}

void rewrite_raw_record(struct DBT *key, struct DBT *data, struct Record *rec)
{
    *key = rec->key;
    *data = rec->data;
}

void rewrite_record_from_raw(struct Record *rec, struct DBT *key, struct DBT *data)
{
    rec->key = *key;
    rec->data = *data;
}

void move_records(int start, int end, int step, struct Record *records_left, struct Record *records_right)
{
    if (start <= end) {
        for (int i = start; i < end; ++i) {
            records_left[i] = records_right[i + step];
        }
    } else {
        for (int i = start; i > end; ++i) {
            records_left[i] = records_right[i + step];
        }
    }
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
