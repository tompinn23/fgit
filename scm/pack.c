#include "pack.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

#include "util.h"
#include "err.h"

struct git_index *git_index_open(const char *file) {
    int fd;
    struct git_index *idx;

    if((fd = open(file, O_RDONLY)) < 0) {
        scm_errno = SCM_ESYSTEM;
        return NULL;
    }

    char magic[4];
    if(xfullread(fd, magic, 4) < 0) {
        scm_errno = SCM_ESYSTEM;
        goto fdclose;
    }

    if(strncmp(magic, "\377tOc", 4) != 0) {
        scm_errno = SCM_EINVALID;
        goto fdclose;
    }

    uint32_t vers;
    if(xread_u32n(fd, &vers) < 0) {
        goto fdclose;
    }

    if(!(vers == 2 || vers == 3)) {
        scm_errno = SCM_EBADVER;
        goto fdclose;
    }

    idx = calloc(1, sizeof(struct git_index));

    uint32_t ent;
    for(int i = 0; i < 256; i++) {
        if(xread_u32n(fd, &ent) < 0) {
            goto fdclose;
        }
        idx->fanout[i] = ent;
    }
    idx->ocount = idx->fanout[255];
    idx->sha1 = calloc(idx->ocount, 20);
    idx->crc32 = calloc(idx->ocount, sizeof(uint32_t));
    idx->offsets = calloc(idx->ocount, sizeof(uint32_t));

    /* read each object sha1 */
    for(int i = 0; i < idx->ocount; i++) {
        if(xfullread(fd, idx->sha1 + (i * 20), 20) < 0) {
            goto err;
        }
    }

    /* read crc 32 checksums */
    for(int i = 0; i < idx->ocount; i++) {
        if(xread_u32n(fd, &idx->crc32[i]) < 0) {
            goto err;
        }
    }


    for(int i = 0; i < idx->ocount; i++) {
        uint32_t offset;
        if(xread_u32n(fd, &offset) < 0) {
            goto err;
        }
        if(offset & 0x80000000) {
            idx->loffsetcount++;
        }
        idx->offsets[i] = offset;
    }

    idx->loffsets = calloc(idx->loffsetcount, sizeof(uint64_t));
    for(int i = 0; i < idx->loffsetcount; i++) {
        if(xread_u64n(fd, &idx->loffsets[i]) < 0) {
            goto err;
        }
    }

    return idx;
err:
    if(idx->sha1 != NULL) {
        free(idx->sha1);
    }
    if(idx->crc32 != NULL) {
        free(idx->crc32);
    }
    free(idx);
fdclose:
    close(fd);
    return NULL;
}

int cmp_sha1(const void *a, const void *b) {
    return memcmp(a, b, 20);
}

int64_t git_index_index(struct git_index *idx, const unsigned char sha1[20]) {
    uint32_t start, end;

    start = (sha1[0] == 0) ? 0 : idx->fanout[sha1[0] - 1];
    end = idx->fanout[sha1[0]];
    if(start == end) {
        return -1;
    }

    void *found = bsearch(sha1, idx->sha1 + start * 20, end - start, 20, cmp_sha1);
    printf("found: %p\n", found);
    if(found == NULL) {
        return -1;
    }

    return ((char *)found - (char *)idx->sha1) / 20;
}

uint64_t git_index_offset(struct git_index *idx, uint32_t index) {
    uint32_t oft = idx->offsets[index];
    if(oft & 0x80000000) {
        return idx->loffsets[oft & 0x7fffffff];
    } else {
        return oft;
    }
}

uint32_t git_index_crc32(struct git_index *idx, uint32_t index) {
    return idx->crc32[index];
}

struct git_pack *git_pack_open(const char *name) {
    struct git_index *gidx;
    struct mapped_file *packfile;
    char *fpath;
    //asprintf(&fpath, "%s.idx", name);
    if(!fpath) {
        return NULL;
    }

    gidx = git_index_open(fpath);
    free(fpath);

    if(!gidx) {
        return NULL;
    }

    //asprintf(&fpath, "%s.pack", name);
    if(!fpath) {
        return NULL;
    }

    packfile = map_file(fpath);
    free(fpath);
    if(!packfile) {
        return NULL;
    }

    struct git_pack *pack;
    pack = calloc(1, sizeof(*pack));

    pack->packfile = packfile;
    pack->idx = gidx;

    return pack;
}

int git_pack_object(struct git_pack *pack, unsigned char sha1[20]) {

    int64_t index = git_index_index(pack->idx, sha1);
    if(index < 0) {
        return -1;
    }
    uint64_t offset = git_index_offset(pack->idx, index);

    if(offset > pack->packfile->length) {
        return -1;
    }

    uint8_t *ptr = pack->packfile->base + offset;
    int type = (*ptr++ >> 4) & 0x07;
    size_t sz = *ptr & 0x0f;
    int shift = 4;
    while(*ptr & 0x80) {
        sz |= (size_t)(*ptr++ & 0x7f) << shift;
        shift += 7;
    }

    z_stream strm = {0};
    if(inflateInit(&strm) != Z_OK) {
        return -1;
    }

    strm.next_in = ptr;

}
