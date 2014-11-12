#define MAX_KEY_SIZE 20
#define MAX_DATA_SIZE 100

typedef unsigned int uint;

enum {
    IS_LEAF = 1
};

struct DBT {
     char *data;
     size_t size;
};

struct DBC {
    size_t db_size;
    size_t chunk_size;
};

struct Record {
    struct DBT key;
    struct DBT data;
    int offset;
};

struct Node {
    uint flags;
    int size;
    int tree_param;
    int offset;
    struct Record records[];
};

struct DB {
    int (*close)(struct DB *db);
    int (*del)(struct DB *db, struct DBT *key);

    int (*get)(struct DB *db, struct DBT *key, struct DBT *data);
    int (*put)(struct DB *db, struct DBT *key, struct DBT *data);

    struct DBC config;

    FILE *db_file;
    int first_free;
    int capacity;
    int root_offset;
    struct Node *root;
    char *bitmask;
};

struct DB *dbcreate(const char *file, const struct DBC conf);
struct DB *dbopen  (const char *file);
void print_tree(struct DB *db, struct Node *node, int k);
