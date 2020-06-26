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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "md5.h"
#include "libimplantisomd5.h"
#include "utilities.h"

static int writeAppData(unsigned char *const appdata, const char *const valstr, size_t *loc, char **errstr) {
    size_t vallen = strlen(valstr);
    if (*loc + vallen >= APPDATA_SIZE) {
        *errstr = "Attempted to write too much appdata.";
        return -1;
    }

    memcpy(appdata + *loc, valstr, vallen);

    *loc += vallen;
    return 0;
}

int implantISOFile(const char *iso, int supported, int forceit, int quiet, char **errstr) {
    int isofd = open(iso, O_RDWR | O_BINARY);
    if (isofd < 0) {
        *errstr = "Error - Unable to open file %s";
        return -1;
    }
    int rc = implantISOFD(isofd, supported, forceit, quiet, errstr);
    close(isofd);
    return rc;
}

int implantISOFD(int isofd, int supported, int forceit, int quiet, char **errstr) {

    off_t pvd_offset;
    const off_t isosize = primary_volume_size(isofd, &pvd_offset);
    if (isosize == 0) {
        *errstr = "Could not find primary volume!";
        return -1;
    }

    lseek(isofd, pvd_offset + APPDATA_OFFSET, SEEK_SET);
    unsigned char appdata[APPDATA_SIZE];
    if (read(isofd, appdata, APPDATA_SIZE) <= 0) {
        *errstr = "Failed to read application data from file.";
        return -errno;
    }

    if (!forceit) {
        for (size_t i = 0; i < APPDATA_SIZE; i++) {
            if (appdata[i] != ' ') {
                *errstr = "Application data has been used - not implanting md5sum!";
                return -1;
            }
        }
    } else {
        /* Write out blanks to erase old app data. */
        lseek(isofd, pvd_offset + APPDATA_OFFSET, SEEK_SET);
        memset(appdata, ' ', APPDATA_SIZE);
        ssize_t error = write(isofd, appdata, APPDATA_SIZE);
        if (error < 0) {
            *errstr = "Write failed.";
            return error;
        }
    }

    /* Rewind, compute md5sum. */
    lseek(isofd, 0LL, SEEK_SET);

    MD5_CTX hashctx;
    MD5_Init(&hashctx);
    char fragmentsums[FRAGMENT_SUM_SIZE + 1];
    *fragmentsums = '\0';

    const size_t pagesize = (size_t) getpagesize();
    const size_t buffer_size = NUM_SYSTEM_SECTORS * SECTOR_SIZE;
    unsigned char *buffer;
    buffer = aligned_alloc(pagesize, buffer_size * sizeof(*buffer));

    const off_t total_size = isosize - SKIPSECTORS * SECTOR_SIZE;
    const off_t fragment_size = total_size / (FRAGMENT_COUNT + 1);
    size_t previous_fragment = 0UL;
    off_t offset = 0LL;
    while (offset < total_size) {
        const size_t nbyte = MIN((size_t)(total_size - offset), buffer_size);
        ssize_t nread = read(isofd, buffer, nbyte);
        if (nread <= 0L)
            break;

        MD5_Update(&hashctx, buffer, (unsigned int) nread);
        const size_t current_fragment = offset / fragment_size;
        const size_t fragmentsize = FRAGMENT_SUM_SIZE / FRAGMENT_COUNT;
        /* If we're onto the next fragment, calculate the previous sum and check. */
        if (current_fragment != previous_fragment) {
            validate_fragment(&hashctx, current_fragment, fragmentsize, NULL, fragmentsums);
            previous_fragment = current_fragment;
        }

        offset += nread;
    }
    free(buffer);

    char hashsum[HASH_SIZE + 1];
    md5sum(hashsum, &hashctx);
    if (!quiet) {
        printf("Inserting md5sum into iso image...\n");
        printf("md5 = %s\n", hashsum);
        printf("Inserting fragment md5sums into iso image...\n");
        printf("fragmd5 = %s\n", fragmentsums);
        printf("frags = %lu\n", FRAGMENT_COUNT);
    }
    memset(appdata, ' ', APPDATA_SIZE);

    size_t loc = 0;
    if (writeAppData(appdata, "ISO MD5SUM = ", &loc, errstr))
        return -1;
    if (writeAppData(appdata, hashsum, &loc, errstr))
        return -1;
    if (writeAppData(appdata, ";", &loc, errstr))
        return -1;

    char appdata_buffer[APPDATA_SIZE];
    snprintf(appdata_buffer, APPDATA_SIZE, "SKIPSECTORS = %lld", SKIPSECTORS);

    if (writeAppData(appdata, appdata_buffer, &loc, errstr))
        return -1;
    if (writeAppData(appdata, ";", &loc, errstr))
        return -1;

    if (!quiet)
        printf("Setting supported flag to %d\n", supported);
    static const char status[] = "RHLISOSTATUS=%d";
    char tmp[sizeof(status) / sizeof(*status)];
    snprintf(tmp, sizeof(status) / sizeof(*status), status, supported);
    if (writeAppData(appdata, tmp, &loc, errstr))
        return -1;

    if (writeAppData(appdata, ";", &loc, errstr))
        return -1;

    if (writeAppData(appdata, "FRAGMENT SUMS = ", &loc, errstr))
        return -1;
    if (writeAppData(appdata, fragmentsums, &loc, errstr))
        return -1;
    if (writeAppData(appdata, ";", &loc, errstr))
        return -1;

    snprintf(appdata_buffer, APPDATA_SIZE, "FRAGMENT COUNT = %lu", FRAGMENT_COUNT);
    if (writeAppData(appdata, appdata_buffer, &loc, errstr))
        return -1;
    if (writeAppData(appdata, ";", &loc, errstr))
        return -1;

    if (writeAppData(
            appdata, "THIS IS NOT THE SAME AS RUNNING MD5SUM ON THIS ISO!!", &loc, errstr))
        return -1;

    if (lseek(isofd, pvd_offset + APPDATA_OFFSET, SEEK_SET) < 0) {
        *errstr = "Seek failed.";
        return -1;
    }

    ssize_t error = write(isofd, appdata, APPDATA_SIZE);
    if (error < 0) {
        *errstr = "Write failed.";
        return -1;
    }

    errstr = NULL;
    return 0;
}
