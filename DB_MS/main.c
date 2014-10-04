#include "lib.h"

int main(void)
{
	printf("start\n");
	struct DB *db;
	struct DBC dbc;
	dbc.db_size = 123;
	dbc.chunk_size = 234;
	dbc.mem_size = 345;
	db = dbopen("database");
	printf("%d\n", (int)(db->getMaxSize(db)));
	printf("%d\n", (int)(db->getCurrentSize(db)));
	printf("%d\n", (int)(db->getChunkSize(db)));
	printf("%d\n", (int)(db->getMemSize(db)));
	int kd = 111;
	int dd = 222;
	struct DBT key;
	key.data = &kd;
	key.size = sizeof(int);
	struct DBT data;
	data.data = &dd;
	data.size = sizeof(int);
	printf("%d\n", db->put(db, &key, &data));
	printf("%d\n", (int)(db->getCurrentSize(db)));
	printf("%d\n", db->close(db));
	return 0;
}