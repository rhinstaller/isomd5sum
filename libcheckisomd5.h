#ifndef __LIBCHECKISOMD5_H__
#define __LIBCHECKISOMD5_H__

#include <sys/types.h>

enum isomd5sum_status {
    ISOMD5SUM_FILE_NOT_FOUND = -2,
    ISOMD5SUM_CHECK_NOT_FOUND = -1,
    ISOMD5SUM_CHECK_FAILED = 0,
    ISOMD5SUM_CHECK_PASSED = 1,
    ISOMD5SUM_CHECK_ABORTED = 2
};

/* For non-zero return value, check is aborted. */
typedef int (*checkCallback)(void *, off_t offset, off_t total);

int mediaCheckFile(char *iso, checkCallback cb, void *cbdata);
int printMD5SUM(char *file);

#endif
