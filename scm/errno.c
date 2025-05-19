#include "errno.h"

_Thread_local int scm_errno_val = 0;

int *__scm_errno_location() {
    return &scm_errno_val;
}

static const char *strs[] = {
    "invalid argument",
    "bad version",
};

const char *scm_strerror(int errno) {
    return strs[errno];
}


