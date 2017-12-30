/*
 * Copyright (C) 2001-2017 Red Hat, Inc.
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
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "./include/md5.h"
#include "./include/utilities.h"

#ifdef _WIN32
#include <windows.h>
#include <malloc.h>

size_t getpagesize() {
    static size_t page_size = 0;
    if (page_size == 0) {
        SYSTEM_INFO system_info;
        GetSystemInfo(&system_info);
        page_size = (size_t)(system_info.dwPageSize);
    }
    return page_size;
}

void *aligned_alloc(size_t alignment, size_t size) {
    return _aligned_malloc(size, alignment);
}
#elif __APPLE__
void *aligned_alloc(size_t alignment, size_t size) {
    void *buffer = NULL;
    posix_memalign(&buffer, alignment, size);
    return buffer;
}
#endif

static unsigned char *read_primary_volume_descriptor(const int fd, off_t *const offset) {
    /*
     * According to ECMA-119 8.1.1.
     */
    enum { BOOT_RECORD = 0,
           PRIMARY = 1,
           ADDITIONAL = 2,
           PARTITION = 3,
           SET_TERMINATOR = 255 };
    off_t nbyte = SYSTEM_AREA_SIZE;
    /* Skip unused system area. */
    if (lseek(fd, nbyte, SEEK_SET) == -1) {
        return NULL;
    }
    unsigned char *sector_buffer;
    sector_buffer = aligned_alloc((size_t) getpagesize(), SECTOR_SIZE * sizeof(*sector_buffer));
    /* Read n volume descriptors. */
    for (;;) {
        if (read(fd, sector_buffer, SECTOR_SIZE) == -1) {
            free(sector_buffer);
            return NULL;
        }
        if (sector_buffer[0] == PRIMARY) {
            break;
        } else if (sector_buffer[0] == SET_TERMINATOR) {
            return NULL;
        }
        nbyte *= SECTOR_SIZE;
    }
    *offset = nbyte;
    return sector_buffer;
}

static off_t isosize(const unsigned char *const buffer) {
    /*
     * Doing multiplications so that it can be guaranteed that the big endian
     * number is converted to the systems endianness without knowing the
     * endianness of the system.
     */
    off_t result = buffer[SIZE_OFFSET] * 0x1000000 +
                   buffer[SIZE_OFFSET + 1] * 0x10000 +
                   buffer[SIZE_OFFSET + 2] * 0x100 + buffer[SIZE_OFFSET + 3];
    result *= SECTOR_SIZE;
    return result;
}

static size_t starts_with(const char *const buffer, const char *const string) {
    const size_t len = strlen(string);
    return strncmp(buffer, string, len) ? 0 : len;
}

/**
 * Read and store number from buffer if the buffer starts with string.
 */
static size_t matches_number(char *const buffer, size_t index,
                             const char *const string, long int *const number) {
    size_t len = starts_with(buffer + index, string);
    index += len;
    if (len > 0UL && index < APPDATA_SIZE) {
        char tmp[APPDATA_SIZE];
        /* The number should be every until the semicolon. */
        char *ptr = tmp;
        for (; index < APPDATA_SIZE && buffer[index] != ';';
             ptr++, index++)
            *ptr = buffer[index];
        *ptr = '\0';
        char *endptr;
        *number = strtol(tmp, &endptr, 10);
        return endptr != NULL && *endptr != '\0' ? 0UL : index;
    }
    return 0UL;
}

off_t primary_volume_size(const int isofd, off_t *const offset) {
    unsigned char *buffer = read_primary_volume_descriptor(isofd, offset);
    if (buffer == NULL)
        return 0;
    off_t tmp = isosize(buffer);
    free(buffer);
    return tmp;
}

/* Find the primary volume descriptor and return parsed information from it. */
struct volume_info *const parsepvd(const int isofd) {
    char buffer[APPDATA_SIZE];
    off_t offset;

    enum task_status {
        TASK_SUPPORTED = 1,
        TASK_FRAGCOUNT = 1 << 1,
        TASK_FRAGSUM = 1 << 2,
        TASK_MD5 = 1 << 3,
        TASK_SKIP = 1 << 4,
        TASK_DONE = (1 << 5) - 1
    };
    enum task_status task = 0;

    unsigned char *const aligned_buffer = read_primary_volume_descriptor(isofd, &offset);
    if (aligned_buffer == NULL)
        return NULL;
    /* Application data */
    memcpy(buffer, aligned_buffer + APPDATA_OFFSET, APPDATA_SIZE);
    buffer[APPDATA_SIZE - 1] = '\0';

    struct volume_info *result = malloc(sizeof(struct volume_info));
    result->skipsectors = SKIPSECTORS;
    result->supported = 0;
    result->fragmentcount = FRAGMENT_COUNT;
    result->offset = offset;
    result->isosize = isosize(aligned_buffer);

    free(aligned_buffer);

    for (size_t index = 0; index < APPDATA_SIZE;) {
        size_t len;
        if ((len = starts_with(buffer + index, "ISO MD5SUM = "))) {
            index += len;
            if (index + HASH_SIZE >= APPDATA_SIZE)
                goto fail;

            memcpy(result->hashsum, buffer + index, HASH_SIZE);
            result->hashsum[HASH_SIZE] = '\0';
            index += HASH_SIZE;
            task |= TASK_MD5;
            /* Skip to next semicolon. */
            for (char *p = buffer + index; index < APPDATA_SIZE && *p != ';';
                 p++, index++) {
            }
        } else if ((len = matches_number(buffer, index,
                                         "SKIPSECTORS = ", (long int *) &result->skipsectors))) {
            index = len;
            if (index >= APPDATA_SIZE)
                goto fail;
            task |= TASK_SKIP;
        } else if ((len = matches_number(buffer, index,
                                         "RHLISOSTATUS=", (long int *) &result->supported))) {
            index = len;
            task |= TASK_SUPPORTED;
        } else if ((len = starts_with(buffer + index, "FRAGMENT SUMS = "))) {
            index += len;
            if (index + FRAGMENT_SUM_SIZE >= APPDATA_SIZE)
                goto fail;
            memcpy(result->fragmentsums, buffer + index, FRAGMENT_SUM_SIZE);
            result->fragmentsums[FRAGMENT_SUM_SIZE] = '\0';
            task |= TASK_FRAGSUM;
            index += FRAGMENT_SUM_SIZE;
            for (char *p = buffer + index; index < APPDATA_SIZE && *p != ';';
                 p++, index++) {
            }
        } else if ((len = matches_number(buffer, index, "FRAGMENT COUNT = ", (long int *) &result->fragmentcount))) {
            index = len;
            task |= TASK_FRAGCOUNT;
        }
        /* Either something is wrong or it skips a semicolon. */
        index++;
        if (task == TASK_DONE)
            break;
    }

    if (task < TASK_SKIP + TASK_MD5) {
    fail:
        free(result);
        return NULL;
    }
    return result;
}

/**
 * Finalize the given hashctx to determine the fragment sum which is:
 * 1. Take the first base 16 character that is not zero from the hashsum byte
 * or zero if the byte holds the value zero.
 * 2. Concatenate as three (in this case) characters as described in 1.
 * Append the fragment sum (in this case the three characters) to hashsums if
 * it's provided.
 * Return if the fragment sum is valid. The excepted fragment sum is provided
 * in fragmentsums which is supposed to be all fragmentsums concatenated.
 * The fragment sum of fragmentsums to compare with is determined by fragment.
 */
bool validate_fragment(const MD5_CTX *const hashctx, const size_t fragment,
                       const size_t fragmentsize, const char *const fragmentsums, char *hashsums) {
    unsigned char digest[HASH_SIZE / 2];
    MD5_CTX ctx;
    memcpy(&ctx, hashctx, sizeof(ctx));
    MD5_Final(digest, &ctx);
    size_t j = (fragment - 1) * fragmentsize;
    for (size_t i = 0; i < MIN(fragmentsize, HASH_SIZE / 2); i++) {
        char tmp[3];
        /*
         * Using "3" to avoid compiler warning about truncation.
         */
        snprintf(tmp, 3, "%01x", digest[i]);
        if (hashsums != NULL)
            strncat(hashsums, tmp, 1);
        if (fragmentsums != NULL && tmp[0] != fragmentsums[j++])
            return false;
    }
    return true;
}

/**
 * Finalize hashctx and store it in hashsum in base 16.
 */
void md5sum(char *const hashsum, MD5_CTX *const hashctx) {
    unsigned char digest[HASH_SIZE / 2];
    MD5_Final(digest, hashctx);
    *hashsum = '\0';
    for (size_t i = 0; i < HASH_SIZE / 2; i++) {
        char tmp[3];
        snprintf(tmp, 3, "%02x", digest[i]);
        strncat(hashsum, tmp, 2);
    }
}
