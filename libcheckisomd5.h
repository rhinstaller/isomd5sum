#ifndef __LIBCHECKISOMD5_H__
#define __LIBCHECKISOMD5_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum isomd5sum_status {
    ISOMD5SUM_FILE_NOT_FOUND = -2,
    ISOMD5SUM_CHECK_NOT_FOUND = -1,
    ISOMD5SUM_CHECK_FAILED = 0,
    ISOMD5SUM_CHECK_PASSED = 1,
    ISOMD5SUM_CHECK_ABORTED = 2
};

/* For non-zero return value, check is aborted. */
typedef int (*checkCallback)(void *, long long offset, long long total);

int mediaCheckFile(const char *file, checkCallback cb, void *cbdata);
int mediaCheckFD(int isofd, checkCallback cb, void *cbdata);
int printMD5SUM(const char *file);

#ifdef __cplusplus
}
#endif

#endif
