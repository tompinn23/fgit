#include "scm/pack.h"
#include "scm/util.h"
#include "scm/repository.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

    struct git_repo *repo = git_repo_open(".");

    printf("head: %s\n", git_repo_head(repo));


    return 0;

    struct git_index *idx = git_index_open("pack-5af864f4568ad9b698c7e51517ded56e9c650a4f.idx");

    uint8_t *buf = hextob("a6ab2b314147c9b6cba6bc70fc947869c82d6df0");

    int64_t index = git_index_index(idx, buf);
    if(index < 0) {
        exit(1);
    }

    printf("index: %lld\n", index);
    printf("%lld (%x)\n", git_index_offset(idx, index), git_index_crc32(idx, index));

    return 0;
}
