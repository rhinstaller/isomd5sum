#ifndef __LIBCHECKISOMD5_H__
#define  __LIBCHECKISOMD5_H__

#define ISOMD5SUM_CHECK_PASSED          1
#define ISOMD5SUM_CHECK_FAILED          0
#define ISOMD5SUM_CHECK_ABORTED         2
#define ISOMD5SUM_CHECK_NOT_FOUND       -1

typedef int (*checkCallback)(void *, long long offset, long long total);

int mediaCheckFile(char *iso, checkCallback cb, void *cbdata);
void printMD5SUM(char *file);

#endif
