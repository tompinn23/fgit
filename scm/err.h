#pragma once

enum {
    SCM_EINVALID,
    SCM_EBADVER,
    SCM_ESYSTEM,
};

#define scm_errno (*__scm_errno_location())

int *__scm_errno_location();

const char *scm_strerror(int errno);
