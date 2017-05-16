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
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "md5.h"
#include "libimplantisomd5.h"
#include "utilities.h"

static size_t writeAppData(unsigned char *const appdata, const char *const valstr, const size_t loc) {
    if (loc + strlen(valstr) >= APPDATA_SIZE) {
        printf("Attempted to write too much appdata, exiting...\n");
        exit(-1);
    }

    memcpy(appdata + loc, valstr, strlen(valstr));

    return loc + strlen(valstr);
}

int implantISOFile(char *fname, int supported, int forceit, int quiet, char **errstr) {
    int isofd = open(fname, O_RDWR | O_BINARY);
    if (isofd < 0) {
        *errstr = "Error - Unable to open file %s\n\n";
        return -1;
    }

    off_t pvd_offset;
    const off_t isosize = primary_volume_size(isofd, &pvd_offset);
    if (isosize == 0) {
        *errstr = "Could not find primary volumne!\n\n";
        return -1;
    }

    lseek(isofd, pvd_offset + APPDATA_OFFSET, SEEK_SET);
    unsigned char appdata[APPDATA_SIZE];
    read(isofd, appdata, APPDATA_SIZE);

    if (!forceit) {
        for (size_t i = 0; i < APPDATA_SIZE; i++) {
            if (appdata[i] != ' ') {
                *errstr = "Application data has been used - not implanting md5sum!\n";
                return -1;
            }
        }
    } else {
        /* Write out blanks to erase old app data. */
        lseek(isofd, pvd_offset + APPDATA_OFFSET, SEEK_SET);
        memset(appdata, ' ', APPDATA_SIZE);
        ssize_t error = write(isofd, appdata, APPDATA_SIZE);
        if (error < 0) {
            printf("write failed %ld\n", error);
            perror("");
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
    size_t previous_fragment = 0UL;
    off_t offset = 0LL;
    while (offset < total_size) {
        const size_t nbyte = MIN((size_t)(total_size - offset), buffer_size);
        ssize_t nread = read(isofd, buffer, nbyte);
        if (nread <= 0L)
            break;

        MD5_Update(&hashctx, buffer, (unsigned int) nread);
        const size_t current_fragment = (size_t) offset * (FRAGMENT_COUNT + 1) / (size_t) total_size;
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
        printf("frags = %ld\n", FRAGMENT_COUNT);
    }
    memset(appdata, ' ', APPDATA_SIZE);

    size_t loc = 0;
    loc = writeAppData(appdata, "ISO MD5SUM = ", loc);
    loc = writeAppData(appdata, hashsum, loc);
    loc = writeAppData(appdata, ";", loc);

    char *appdata_buffer;
    appdata_buffer = aligned_alloc(pagesize, APPDATA_SIZE * sizeof(*appdata_buffer));
    snprintf(appdata_buffer, APPDATA_SIZE, "SKIPSECTORS = %lld", SKIPSECTORS);

    loc = writeAppData(appdata, appdata_buffer, loc);
    loc = writeAppData(appdata, ";", loc);

    if (!quiet)
        printf("Setting supported flag to %d\n", supported);
    static const char status[] = "RHLISOSTATUS=%d";
    char tmp[sizeof(status) / sizeof(*status)];
    snprintf(tmp, sizeof(status) / sizeof(*status), status, supported);
    loc = writeAppData(appdata, tmp, loc);

    loc = writeAppData(appdata, ";", loc);

    loc = writeAppData(appdata, "FRAGMENT SUMS = ", loc);
    loc = writeAppData(appdata, fragmentsums, loc);
    loc = writeAppData(appdata, ";", loc);

    snprintf(appdata_buffer, APPDATA_SIZE, "FRAGMENT COUNT = %ld", FRAGMENT_COUNT);
    loc = writeAppData(appdata, appdata_buffer, loc);
    loc = writeAppData(appdata, ";", loc);

    loc = writeAppData(
        appdata, "THIS IS NOT THE SAME AS RUNNING MD5SUM ON THIS ISO!!", loc);

    if (lseek(isofd, pvd_offset + APPDATA_OFFSET, SEEK_SET) < 0)
        printf("seek failed\n");

    ssize_t error = write(isofd, appdata, APPDATA_SIZE);
    if (error < 0) {
        printf("write failed %ld\n", error);
        perror("");
    }

    close(isofd);
    errstr = NULL;
    return 0;
}
