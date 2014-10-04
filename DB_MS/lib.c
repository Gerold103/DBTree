#include "lib.h"
#include <string.h>
//----------------------------------------------------------D A T A   B A S E----------------------------------------------------------

//--------D A T A   B A S E   P R I V A T E   S E C T I O N

struct __DB_private__ {
	size_t max_size;
	size_t current_size;
	size_t chunk_size;
	size_t mem_size;
	FILE *db_file;
};

size_t getMaxSize(const struct DB *db)
{
	return db->private->max_size;
}

size_t getCurrentSize(const struct DB *db)
{
	return db->private->current_size;
}

size_t getChunkSize(const struct DB *db)
{
	return db->private->chunk_size;
}

size_t getMemSize(const struct DB *db)
{
	return db->private->mem_size;
}

//---------------------------------------------------------

int close(struct DB *db)
{
	if (db == NULL) {
		return 0;
	}
	if (fclose(db->private->db_file) == EOF) {
		return 0;
	}
	free(db->private);
	free(db);
	return 1;	
}

int put(const struct DB *db, struct DBT *key, const struct DBT *data)
{
	if (db->private->current_size + key->size + data->size > db->private->max_size) {
		return 0;
	}
	if ((db == NULL) || (key == NULL)) {
		return 0;
	}
	if (fseek(db->private->db_file, 0, SEEK_END)) {
		return 0;
	}
	void *node = (void *)malloc(key->size + data->size);
	if (node == NULL) {
		return 0;
	}
	memcpy(node, key->data, key->size);
	memcpy(node + key->size, data->data, data->size);
	if ((int)fwrite(node, key->size + data->size, 1, db->private->db_file) != key->size + data->size) {
		return 0;
	}
	db->private->current_size += key->size + data->size;
	return 1;
}

struct DB *dbcreate(const char *file, const struct DBC *conf)
{
	if ((file == NULL) || (conf == NULL)) {
		return NULL;
	}
	int i;
	//neccessary instances creating

	FILE *db_file = fopen(file, "w+");
	if (db_file == NULL) {
		//exit if "fopen()" failed
		return NULL;
	}
	struct DB *db_res = (struct DB *)malloc(sizeof(struct DB));
	if (db_res == NULL) {
		//exit if "malloc() for "struct DB" falied
		fclose(db_file);
		return NULL;
	}

	//private section initializing

	db_res->private = (struct __DB_private__ *)malloc(sizeof(struct __DB_private__));
	if (db_res->private == NULL) {
		//exit if "malloc()" for private section falied
		free(db_res);
		fclose(db_file);
		return NULL;
	}
	db_res->private->max_size = conf->db_size;
	db_res->private->current_size = 0;
	db_res->private->chunk_size = conf->chunk_size;
	db_res->private->mem_size = conf->mem_size;
	db_res->private->db_file = db_file;

	//current data base settings saving
	size_t size_t_size = sizeof(size_t);
	size_t results[4];

	size_t *max_size = &db_res->private->max_size;
	size_t *current_size = &db_res->private->current_size;
	size_t *chunk_size = &db_res->private->chunk_size;
	size_t *mem_size = &db_res->private->mem_size;
	results[0] = fwrite(max_size, size_t_size, 1, db_file);
	results[1] = fwrite(current_size, size_t_size, 1, db_file);
	results[2] = fwrite(chunk_size, size_t_size, 1, db_file);
	results[3] = fwrite(mem_size, size_t_size, 1, db_file);
	for (i = 0; i < 4; ++i) {
		if (results[i] != size_t_size) {
			//exit because of error during settings saving
			free(db_res->private);
			free(db_res);
			fclose(db_file);
			return NULL;
		}
	}

	db_res->getMaxSize = getMaxSize;
	db_res->getCurrentSize = getCurrentSize;
	db_res->getChunkSize = getChunkSize;
	db_res->getMemSize = getMemSize;
	db_res->close = close;
	db_res->put = put;
	return db_res;
}

struct DB *dbopen(const char *file)
{
	if (file == NULL) {
		return NULL;
	}
	struct DB *db_res;
	FILE *db_file;

	//neccessary instances creating

	db_file = fopen(file, "r+");
	if (db_file == NULL) {
		//exit if "file" doesn't exist
		return NULL;
	}

	db_res = (struct DB *)malloc(sizeof(struct DB));
	if (db_res == NULL) {
		//exit if "malloc()" for "struct DB" failed
		fclose(db_file);
		return NULL;
	}

	db_res->private = (struct __DB_private__ *)malloc(sizeof(struct __DB_private__));
	if (db_res->private == NULL) {
		//exit if "malloc()" for private section failed
		fclose(db_file);
		free(db_res);
		return NULL;
	}

	//data base reading

	size_t size_t_size = sizeof(size_t);
	size_t max_size = 0;
	size_t current_size = 0;
	size_t chunk_size = 0;
	size_t mem_size = 0;
	fread(&max_size, size_t_size, 1, db_file);
	fread(&current_size, size_t_size, 1, db_file);
	fread(&chunk_size, size_t_size, 1, db_file);
	fread(&mem_size, size_t_size, 1, db_file);
	
	db_res->private->max_size = max_size;
	db_res->private->current_size = current_size;
	db_res->private->chunk_size = chunk_size;
	db_res->private->mem_size = mem_size;
	db_res->private->db_file = db_file;

	db_res->getMaxSize = getMaxSize;
	db_res->getCurrentSize = getCurrentSize;
	db_res->getChunkSize = getChunkSize;
	db_res->getMemSize = getMemSize;
	db_res->close = close;
	db_res->put = put;
	return db_res;
}