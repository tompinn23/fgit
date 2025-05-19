#include "util.h"

#include <ctype.h>
#include <sys/endian.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>


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
        scm_errno = SCM_ESYSTEM;
        return -1;
    }
    *val = read_u32n(buf);
    return 0;
}

int xread_u64n(int fd, uint64_t *val) {
    char buf[8];
    if(xfullread(fd, buf, 8) < 0) {
        scm_errno = SCM_ESYSTEM;
        return -1;
    }
    *val = read_u64n(buf);
    return 0;
}

uint32_t read_u32n(const char *buf) {
    uint32_t nval;
    memcpy(&nval, buf, sizeof(uint32_t));
    return betoh32(nval);
}

uint64_t read_u64n(const char *buf) {
    uint64_t nval;
    memcpy(&nval, buf, sizeof(uint64_t));
    return betoh64(nval);
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
