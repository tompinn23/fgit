#pragma once

#include "pack.h"

struct git_repo {
    int gitfd;
    char *path;
    struct git_pack **packs;
    size_t npacks;
};

struct git_repo *git_repo_open(const char *path);
unsigned char *git_repo_head(struct git_repo *repo);