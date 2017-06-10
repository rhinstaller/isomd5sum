#ifndef __LIBIMPLANTISOMD5_H__
#define __LIBIMPLANTISOMD5_H__

#ifdef __cplusplus
extern "C" {
#endif

int implantISOFile(const char *iso, int supported, int forceit, int quiet, char **errstr);
int implantISOFD(int isofd, int supported, int forceit, int quiet, char **errstr);

#ifdef __cplusplus
}
#endif

#endif

