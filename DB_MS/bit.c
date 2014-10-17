#include "bit.h"
#include <limits.h>

bool bit_test(const void *data, size_t pos)
{
    size_t chunk = pos / CHAR_BIT;
    size_t offset = pos % CHAR_BIT;

    const unsigned char *cdata = (const unsigned char  *) data;
    return (cdata[chunk] >> offset) & 0x1;
}

bool bit_set(void *data, size_t pos)
{
    size_t chunk = pos / CHAR_BIT;
    size_t offset = pos % CHAR_BIT;

    unsigned char *cdata = (unsigned char  *) data;
    bool prev = (cdata[chunk] >> offset) & 0x1;
    cdata[chunk] |= (1U << offset);
    return prev;
}
