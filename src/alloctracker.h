/**
 * alloctracker - track dynamic memory allocations and open files
 * Copyright (C) 2019-2020 Daniel Haase
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at
 * https://www.boost.org/LICENSE_1_0.txt
 *
 * The project is inspired by code written by my colleague Jan Z.
 *
 * File:    alloctracker.h
 * Author:  Daniel Haase
 *
 */

#ifndef _AT_ALLOC_TRACKER_H_
#define _AT_ALLOC_TRACKER_H_

#ifdef __cplusplus
extern C
{
#endif

#ifdef AT_ALLOC_TRACK

#include <string.h>
#include "alloctracker_intern.h"

#define AT_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#define AT_FUNCTION __func__
#else
#define AT_FUNCTION ""
#endif

#undef malloc
#define malloc(X) at_malloc((X), (AT_FILENAME), AT_FUNCTION, __LINE__)
#undef calloc
#define calloc(X, Y) at_calloc((X), (Y), (AT_FILENAME), AT_FUNCTION, __LINE__)
#undef realloc
#define realloc(P, X) at_realloc((P), (X), (AT_FILENAME), AT_FUNCTION, __LINE__)
#undef free
#define free(P) at_free((P))

#if defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 500 \
 || defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200809L \
 || defined _BSC_SOURCE || defined _SVID_SOURCE
#undef strdup
#define strdup(S) at_strdup((S), (AT_FILENAME), AT_FUNCTION, __LINE__)
#endif

#if defined __USE_XOPEN2K8 || __GLIBC_USE(LIB_EXT2) \
 || defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200809L \
 || defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 700 \
 || defined _GNU_SOURCE
#undef getline
#define getline(L, S, F) at_getline((L), (S), (F), (AT_FILENAME), AT_FUNCTION, __LINE__)
#undef getdelim
#define getdelim(L, S, D, F) at_getdelim((L), (S), (D), (F), (AT_FILENAME), AT_FUNCTION, __LINE__)
#endif

#undef fopen
#define fopen(N, M) at_fopen((N), (M), (AT_FILENAME), AT_FUNCTION, __LINE__)
#undef freopen
#define freopen(N, M, F) at_freopen((N), (M), (F), (AT_FILENAME), AT_FUNCTION, __LINE__)
#undef tmpfile
#define tmpfile() at_tmpfile((AT_FILENAME), AT_FUNCTION, __LINE__)
#undef fclose
#define fclose(F) at_fclose((F))

#define AT_REPORT at_report()
#define AT_FREE_ALL at_free_all()

#else

#define AT_REPORT
#define AT_FREE_ALL

#endif

char *at_version(void);

#ifdef __cplusplus
}
#endif

#endif
