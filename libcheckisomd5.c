/*
 * Copyright (C) 2001-2013 Red Hat, Inc.
 *
 * Michael Fulbright <msf@redhat.com>
 * Dustin Kirkland  <dustin.dirkland@gmail.com>
 *      Added support for checkpoint fragment sums;
 *      Exits media check as soon as bad fragment md5sum'ed
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "md5.h"
#include "libcheckisomd5.h"
#include "utilities.h"

static void clear_appdata(unsigned char *const buffer, const size_t size, const off_t appdata_offset, const off_t offset) {
    static const ssize_t buffer_start = 0;
    const ssize_t difference = appdata_offset - offset;
    if (-APPDATA_SIZE <= difference && difference <= (ssize_t) size) {
        const size_t clear_start = (size_t) MAX(buffer_start, difference);
        const size_t clear_len = MIN(size, (size_t)(difference + APPDATA_SIZE)) - clear_start;
        memset(buffer + clear_start, ' ', clear_len);
    }
}

static enum isomd5sum_status checkmd5sum(int isofd, checkCallback cb, void *cbdata) {
    struct volume_info *const info = parsepvd(isofd);
    if (info == NULL)
        return ISOMD5SUM_CHECK_NOT_FOUND;

    const off_t total_size = info->isosize - info->skipsectors * SECTOR_SIZE;
    if (cb)
        cb(cbdata, 0, total_size);

    /* Rewind, compute md5sum. */
    lseek(isofd, 0LL, SEEK_SET);

    MD5_CTX hashctx;
    MD5_Init(&hashctx);

    const size_t buffer_size = NUM_SYSTEM_SECTORS * SECTOR_SIZE;
    unsigned char *buffer;
    buffer = aligned_alloc((size_t) getpagesize(), buffer_size * sizeof(*buffer));

    size_t previous_fragment = 0UL;
    off_t offset = 0LL;
    while (offset < total_size) {
        const size_t nbyte = MIN((size_t)(total_size - offset), buffer_size);

        ssize_t nread = read(isofd, buffer, nbyte);
        if (nread <= 0L)
            break;

        /**
         * Originally was added in 2005 because the kernel was returning the
         * size from where it started up to the end of the block it pre-fetched
         * from a cd drive.
         */
        if (nread > nbyte) {
            nread = nbyte;
            lseek(isofd, offset + nread, SEEK_SET);
        }
        /* Make sure appdata which contains the md5sum is cleared. */
        clear_appdata(buffer, nread, info->offset + APPDATA_OFFSET, offset);

        MD5_Update(&hashctx, buffer, (unsigned int) nread);
        if (info->fragmentcount) {
            const size_t current_fragment = (size_t)(offset * (off_t)(info->fragmentcount + 1) / total_size);
            const size_t fragmentsize = FRAGMENT_SUM_SIZE / info->fragmentcount;
            /* If we're onto the next fragment, calculate the previous sum and check. */
            if (current_fragment != previous_fragment) {
                if (!validate_fragment(&hashctx, current_fragment, fragmentsize,
                                       info->fragmentsums, NULL)) {
                    /* Exit immediately if current fragment sum is incorrect */
                    free(info);
                    free(buffer);
                    return ISOMD5SUM_CHECK_FAILED;
                }
                previous_fragment = current_fragment;
            }
        }
        offset += nread;
        if (cb)
            if (cb(cbdata, offset, total_size)) {
                free(info);
                free(buffer);
                return ISOMD5SUM_CHECK_ABORTED;
            }
    }
    free(buffer);

    if (cb)
        cb(cbdata, info->isosize, total_size);

    char hashsum[HASH_SIZE + 1];
    md5sum(hashsum, &hashctx);

    int failed = strcmp(info->hashsum, hashsum);
    free(info);
    return failed ? ISOMD5SUM_CHECK_FAILED : ISOMD5SUM_CHECK_PASSED;
}

int mediaCheckFile(char *file, checkCallback cb, void *cbdata) {
    int isofd = open(file, O_RDONLY | O_BINARY);
    if (isofd < 0) {
        return ISOMD5SUM_FILE_NOT_FOUND;
    }
    return checkmd5sum(isofd, cb, cbdata);
}

int printMD5SUM(char *file) {
    int isofd = open(file, O_RDONLY | O_BINARY);
    if (isofd < 0) {
        return ISOMD5SUM_FILE_NOT_FOUND;
    }
    struct volume_info *const info = parsepvd(isofd);
    close(isofd);
    if (info == NULL) {
        return ISOMD5SUM_CHECK_NOT_FOUND;
    }

    printf("%s:   %s\n", file, info->hashsum);
    if (strlen(info->fragmentsums) > 0 && info->fragmentcount > 0) {
        printf("Fragment sums: %s\n", info->fragmentsums);
        printf("Fragment count: %zu\n", info->fragmentcount);
        printf("Supported ISO: %s\n", info->supported ? "yes" : "no");
    }
    free(info);
    return 0;
}
