#pragma once

#include <stddef.h>
#include <stdint.h>


struct mapped_file {
    void  *base;
    size_t length;
};

int xfullread(int fd, char *buf, size_t amt);

int xread_u32n(int fd, uint32_t *val);
int xread_u64n(int fd, uint64_t *val);

uint32_t read_u32n(const char *buf);
uint64_t read_u64n(const char *buf);

unsigned char *hextob(const char *sha1);

struct mapped_file *map_file(const char *fname);
