
#include "repository.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"

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

    repo = malloc(sizeof(*repo));
    if(!repo) {
        goto err;
    }
    repo->gitfd = fd;
    repo->path = path;

    return repo;
err:
    if(fd > 0) {
        close(fd);
    }
    return NULL;
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

unsigned char *git_repo_head(struct git_repo *repo) {
    FILE *fp = fopenat(repo->gitfd, "HEAD", "r");
    char *line = NULL;
    size_t len;

    if(!fp) {
        return NULL;
    }

    line = areadline(fp);
    if(line == NULL) {
        fclose(fp);
        return NULL;
    }

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

    free(line);
    free(ref[0]);

    char *ret = read_sha1(repo, ref[1]);
    if(ret != NULL) {
        free(ref[1]);
        return ret;
    }

    fp = fopenat(repo->gitfd, "packed-refs", "r");
    char linebuf[4096];
    ssize_t linesz;
    char *sha1 = NULL;
    while((linesz = readline(fp, linebuf, sizeof(linebuf))) != -1) {
        char *packedref[2] = {0};

        if(linebuf[0] == '#') continue;
        if(strsplit(linebuf, ' ', 2, packedref) != 2) {
            continue;
        }
        if(strcmp(ref[1], packedref[1]) == 0) {
            sha1 = strdup(packedref[0]);
            break;
        }
    }
    fclose(fp);
    return sha1;
}
