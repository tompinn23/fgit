#pragma once

#include <unistd.h>

#include "util.h"

struct git_index {
    uint32_t ocount;
    uint32_t fanout[256];
    char *sha1;
    uint32_t *crc32;
    uint32_t *offsets;
    uint32_t loffsetcount;
    uint64_t *loffsets;
};

struct git_pack {
    struct git_index *idx;
    struct mapped_file *packfile;
};

struct git_index *git_index_open(const char *file);
int64_t git_index_index(struct git_index *idx, const unsigned char sha1[20]);
uint64_t git_index_offset(struct git_index *idx, uint32_t index);
uint32_t git_index_crc32(struct git_index *idx, uint32_t index);

struct git_pack *git_pack_open(const char *name);
