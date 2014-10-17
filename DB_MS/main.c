#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "lib.h"
#include "bit.h"

int main(void)
{
    struct DBC conf;
    conf.db_size = 1024 * 1024 * 128;
    conf.page_size = 512;
    struct DB *db = dbopen("database");
    printDB(db);
    return 0;
}

