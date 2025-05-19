#include "scm/pack.h"
#include "scm/util.h"

#include <stdio.h>

int main(int argc, char **argv) {

    struct git_idx *idx = git_idx_open("pack-5af864f4568ad9b698c7e51517ded56e9c650a4f.idx");

    uint8_t *buf = hextob("a6ab2b314147c9b6cba6bc70fc947869c82d6df0");

    int64_t index = git_idx_index(idx, buf);
    if(index < 0) {
        exit(1);
    }

    printf("index: %lld\n", index);
    printf("%lld (%x)\n", git_idx_offset(idx, index), git_idx_crc32(idx, index));

    return 0;
}
