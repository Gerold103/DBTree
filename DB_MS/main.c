#include <stdio.h>
#include <string.h>
#include "btree.h"

void init(struct DBT *dbt, const char *data)
{
    dbt->data = (char *)malloc(strlen(data) * sizeof(char));
    strcpy(dbt->data, data);
    dbt->size = strlen(data) * sizeof(char);
}

int main()
{
    struct DBC conf;
    conf.chunk_size = 4096;
    conf.db_size = 400096;
    struct DB *db = dbcreate("mydb.txt", conf);
    struct DBT keys[3];
    struct DBT data[3];

    init(&(keys[0]), "key0");
    init(&(keys[1]), "key1");
    init(&(keys[2]), "key2");

    init(&(data[0]), "data0");
    init(&(data[1]), "data1");
    init(&(data[2]), "data2");

    for (int i = 0; i < 3; ++i) {
        db->put(db, &keys[i], &data[i]);
    }
    print_tree(db, db->root, 1);

    printf("\n\n");

    struct DBT anotherkey, anotherdata;

    init(&anotherkey, "key1");
    init(&anotherdata, "datadatadata");
    db->put(db, &anotherkey, &anotherdata);
    print_tree(db, db->root, 1);

    db->del(db, &(keys[0]));
    db->del(db, &anotherkey);
    print_tree(db, db->root, 1);


    free(keys[0].data);
    free(keys[1].data);
    free(keys[2].data);
    free(data[0].data);
    free(data[1].data);
    free(data[2].data);
    return 0;
}
