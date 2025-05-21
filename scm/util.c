#define _DEFAULT_SOURCE
#include "util.h"

#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <endian.h>
#include <stdio.h>


#include "err.h"

int xfullread(int fd, char *buf, size_t amt) {
    size_t total = 0;
    int rc;
    while(1) {
        rc = read(fd, buf + total, amt - total);
        if(rc < 0) {
            if(errno == EINTR) {
                continue;
            }
            return rc;
        }
        total += rc;
        if(total == amt) {
            return total;
        }
    }
    return -1;
}

int xread_u32n(int fd, uint32_t *val) {
    char buf[4];
    if(xfullread(fd, buf, 4) < 0) {
        scm_errno = SCM_EERROR;
        return -1;
    }
    *val = read_u32n(buf);
    return 0;
}

int xread_u64n(int fd, uint64_t *val) {
    char buf[8];
    if(xfullread(fd, buf, 8) < 0) {
        scm_errno = SCM_EERROR;
        return -1;
    }
    *val = read_u64n(buf);
    return 0;
}

uint32_t read_u32n(const char *buf) {
    uint32_t nval;
    memcpy(&nval, buf, sizeof(uint32_t));
    return be32toh(nval);
}

uint64_t read_u64n(const char *buf) {
    uint64_t nval;
    memcpy(&nval, buf, sizeof(uint64_t));
    return be64toh(nval);
}

unsigned char *hextob(const char *sha1) {
    if(!sha1) return NULL;
    if(strlen(sha1) % 2 != 0) return NULL;

    size_t blen = strlen(sha1) / 2;

    char *buf = malloc(blen);
    if(!buf) return NULL;

    for (size_t i = 0; i < blen; i++) {
        char byte_str[3] = { sha1[i * 2], sha1[i * 2 + 1], '\0' };
        if (!isxdigit(byte_str[0]) || !isxdigit(byte_str[1])) {
            free(buf);
            return NULL;
        }
        buf[i] = (uint8_t)strtol(byte_str, NULL, 16);
    }

    return buf;
}

struct mapped_file *map_file(const char *fname) {
    int fd;
    struct mapped_file *map;
    struct stat st;

    fd = open(fname, O_RDONLY);
    if(fd < 0) {
        return NULL;
    }

    if(fstat(fd, &st) < 0) {
        close(fd);
        return NULL;
    }

    void *ptr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if(ptr == MAP_FAILED) return NULL;

    map = calloc(1, sizeof(*map));
    if(!map) {
        munmap(ptr, st.st_size);
        return NULL;
    }
    map->base = ptr;
    map->length = st.st_size;

    return map;
}

int fdpath(int fd, char *buf, size_t bufsz) {
    char path[4096];
#if defined(__linux__)
    snprintf(path, 4096, "/proc/self/fd/%d", fd);
    return readlink(path, buf, bufsz);
#elif defined(__OpenBSD__)
    snprintf(path, 4096, "/dev/fd/%d", fd);
    return readlink(path, buf, bufsz);
#endif
}

FILE *fopenat(int dfd, const char *path, const char *mode) {
    int flags = 0;
    int fd = -1;

    if(mode[0] == 'r') {
        flags = O_RDONLY;
    } else if(mode[0] == 'w') {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if(mode[0] == 'a') {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    }

    if(strchr(mode, '+') != NULL) {
        flags &= ~(O_WRONLY | O_RDONLY);
        flags |= O_RDWR;
    }

    printf("dfd: %d flags: %s path: %s\n", dfd, mode, path);


    fd = openat(dfd, path, flags);
    if(fd < 0) {
        char buf[1024];
        int err = errno;
        if(fdpath(dfd, buf, sizeof(buf)) >= 0) {
            fprintf(stderr, "fullpath: %s/%s\n", buf, path);
        }
        fprintf(stderr, "err: fopenat %s\n", strerror(err));
        return NULL;
    }

    FILE *fp = fdopen(fd, mode);
    if(!fp) {
        close(fd);
    }

    return fp;
}

char *areadline(FILE *fp) {
    char *buf = NULL;
    size_t len = 0;
    ssize_t ret = getline(&buf, &len, fp);
    if(buf == NULL || ret == -1) { return NULL; }

    while (ret > 0 && (buf[ret - 1] == '\n' || buf[ret - 1] == '\r')) {
        buf[--ret] = '\0';
    }
    return buf;
}

ssize_t readline(FILE *fp, char *buf, size_t len) {
    if(buf == NULL || len == 0) return 0;
    if(fp == NULL) return -1;

    size_t i = 0;
    int ch;

    while(i < len - 1) {
        ch = fgetc(fp);
        if(ch == EOF) break;
        if(ch == '\n') break;

        buf[i++] = ch;
    }

    buf[i] = '\0';
    if(i == 0 && ch == EOF) return -1;

    return i;
}

int strsplit(char *s, char delim, int max, char **ss) {
    char *p = s;
    for(int i = 0; i < max; i++) {
        char *ret = strchr(p, delim);
        if(ret != NULL) {
            *ret = '\0';
            ss[i] = p;
            p = ret + 1;
        } else {
            ss[i] = p;
            return i + 1;
        }
    }
    return -1;
}

char *trim(char *s) {
    char *start = s;

    while(isspace(*start)) start++;

    size_t len = strlen(start);
    while(len > 0 && isspace(*(start + len))) len--;
    if(len == 0) {
        return NULL;
    }
    return strndup(start, len);
}

int strtrimsplit(char *s, char delim, int max, char **ss) {
    char *p = s;
    for(int i = 0; i < max; i++) {
        char *ret = strchr(p, delim);
        if(ret != NULL) {
            *ret = '\0';
            ss[i] = trim(p);
            p = ret + 1;
        } else {
            ss[i] = trim(p);
            return i + 1;
        }
    }
    return -1;
}


