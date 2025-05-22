#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


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
FILE *fopenat(int dfd, const char *path, const char *mode);

char *readlinea(FILE *fp);
ssize_t readline(FILE *fp, char *buf, size_t len);

/** This destructively modifies the original input buffer by inserting nulls */
int strsplit(char *s, char delim, int max, char **ss);
/** this copies from the input buffer the returned splits must be freed after use. */
int strtrimsplit(char *s, char delim, int max, char **ss);
