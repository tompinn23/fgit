#pragma once

#define SCM_EERROR  (-1)
#define SCM_ECHKSUM (-2)
#define SCM_ENOMEM  (-3)
#define SCM_EDATA   (-4)

#define scm_errno (*__scm_errno_location())

int *__scm_errno_location();

const char *scm_strerror(int errno);
