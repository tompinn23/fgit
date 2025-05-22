
#include "repository.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "util.h"

#define OBJ_COMMIT (1)
#define OBJ_TREE (2)
#define OBJ_BLOB (3)
#define OBJ_TAG (4)
#define OBJ_OFS_DELTA (6)
#define OBJ_REF_DELTA (7)

static char *get_gitdir(const char *path) {
    char pathbuf[4096];
    struct stat st;
    snprintf(pathbuf, 4096, "%s/HEAD", path);

    if(lstat(pathbuf, &st) == 0 && S_ISREG(st.st_mode)) {
        return strdup(path);
    }

    snprintf(pathbuf, 4096, "%s/.git/HEAD", path);
    if(lstat(pathbuf, &st) == 0 && S_ISREG(st.st_mode)) {
        char *out = malloc(strlen(path) + 6);
        if(!out) { return NULL; }
        snprintf(out, strlen(path) + 6, "%s/.git", path);
        return out;
    }

    return NULL;
}


struct git_repo *git_repo_open(const char *file) {
    struct git_repo *repo;
    int fd = -1;
    char *path = get_gitdir(file);
    if(path == NULL) {
        goto err;
    }

    printf("attempting git path: %s\n", path);
    fd = open(path, O_DIRECTORY | O_NOFOLLOW);
    if(fd < 0) {
        goto err;
    }

    repo = calloc(1, sizeof(*repo));
    if(!repo) {
        goto err;
    }
    repo->gitfd = fd;
    repo->path = path;
    
    fd = openat(repo->gitfd, "objects/pack", O_DIRECTORY | O_NOFOLLOW);
    if(fd < 0) {
        goto err2;
    }

    DIR *dp = fdopendir(fd);
    struct dirent *ent;
    if(dp) {
        while((ent = readdir(dp)) != NULL) {
            char *end = strrchr(ent->d_name, '.');
            char buff[4096];
            if(!strncmp("pack-", ent->d_name, 5) && end != NULL && !strcmp(end + 1, "idx")) {
                snprintf(buff, 4096, "objects/pack/%.*s", (end - ent->d_name), ent->d_name);
                struct git_pack *pk = git_pack_openat(repo->gitfd, buff);
                if(pk != NULL) {
                    void *nw = realloc(repo->packs, repo->npacks + 1);
                    if(!nw) {
                        goto err2;
                    }
                    repo->packs = nw;
                    repo->npacks++;
                    repo->packs[repo->npacks - 1] = pk;
                }
            }
        }
    }
    closedir(dp);
    return repo;
err2:
    git_repo_close(repo);
    return NULL;
err:
    if(fd >= 0) {
        close(fd);
    }
    free(path);
    return NULL;
}

void git_repo_close(struct git_repo *repo) {
    if(repo->gitfd >= 0) close(repo->gitfd);
    for(int i = 0; i < repo->npacks; i++) {
        git_pack_close(repo->packs[i]);
    }
    free(repo->path);
    free(repo);
}

unsigned char *read_sha1(struct git_repo *repo, const char *path) {
    FILE *fp = fopenat(repo->gitfd, path, "r");
    if(!fp) return NULL;
    /* size of sha1 hex plus null */
    size_t len = 41;
    char *buf = malloc(len);

    int ret = readline(fp, buf, 41);
    fclose(fp);
    return ret < 0 ? NULL : buf;
}

unsigned char *git_repo_ref(struct git_repo *repo, char *ref) {
    char *ret = read_sha1(repo, ref);
    if(ret != NULL) {
        return ret;
    }

    FILE *fp = fopenat(repo->gitfd, "packed-refs", "r");
    char linebuf[4096];
    ssize_t linesz;
    char *sha1 = NULL;
    while((linesz = readline(fp, linebuf, sizeof(linebuf))) != -1) {
        char *packedref[2] = {0};

        if(linebuf[0] == '#') continue;
        if(strsplit(linebuf, ' ', 2, packedref) != 2) {
            continue;
        }
        if(strcmp(ref, packedref[1]) == 0) {
            sha1 = strdup(packedref[0]);
            break;
        }
    }
    fclose(fp);
    return sha1;
}

unsigned char *git_repo_head(struct git_repo *repo) {
    FILE *fp = fopenat(repo->gitfd, "HEAD", "r");
    char *line = NULL;
    size_t len;

    if(!fp) {
        return NULL;
    }

    line = readlinea(fp);
    if(line == NULL) {
        fclose(fp);
        return NULL;
    }

    /* these are just pointers into line above */
    char *ref[2] = {0};
    if(strtrimsplit(line, ':', 2, ref) != 2) {
        fclose(fp);
        return NULL;
    }
    if(strcmp("ref", ref[0]) != 0) {
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    unsigned char *ret = git_repo_ref(repo, ref[1]);

    free(line);
    return ret;
}

#define DELTA_MAX_SIZE 64

struct obj {
    int type;
    uint8_t *data;
    size_t len;
};


int git_repo_objdelta(struct git_repo *repo, int type, char *initialbuf, size_t initialsz, char **buf, size_t *bufsz) {
    struct obj stack[DELTA_MAX_SIZE];
    int i = 0;
    while(i < DELTA_MAX_SIZE) {

    }
}

int git_repo_object(struct git_repo *repo, unsigned char sha1[20], char **buf, size_t *bufsz) {
    int64_t rc;
    int type;
    char buff;
    size_t buffsz;
    for(int i = 0; i < repo->npacks; i++) {
        rc = git_pack_blob(repo->packs[i], sha1, &type, &buff);
        if(rc > 0) {
            break;
        }
        buffsz = rc;
    }
    if(type == OBJ_OFS_DELTA || type == OBJ_REF_DELTA) {
        git_repo_objdelta(repo, type, buff, buffsz, &buf, &bufsz);
    }
}
