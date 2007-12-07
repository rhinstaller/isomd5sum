#ifndef __LIBCHECKISOMD5_H__
#define  __LIBCHECKISOMD5_H__

typedef void (*checkCallback)(void *, long long offset, long long total);

int mediaCheckFile(char *iso, checkCallback cb, void *cbdata);
void printMD5SUM(char *file);

#endif
