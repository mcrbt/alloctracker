/**
 * alloctracker - track dynamic memory allocations and open files
 * Copyright (C) 2019-2020 Daniel Haase
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at
 * https://www.boost.org/LICENSE_1_0.txt
 *
 * The project is inspired by code written by my colleague Jan Z.
 *
 * File:    alloctracker_intern.h
 * Author:  Daniel Haase
 *
 */

#ifndef _AT_ALLOC_TRACKER_INTERN_H_
#define _AT_ALLOC_TRACKER_INTERN_H_

#ifndef ALLOC_TRACKER_VERSION
#define ALLOC_TRACER_VERSION "0.5.3"
#endif

#ifdef __cplusplus
extern C
{
#endif

#include <stdio.h>
#include <stdlib.h>

#define at_free_null(p) if(p) { free(p); p = NULL; }

typedef struct at_track_stats
{
	size_t alloc_amount;
	size_t alloc_no;
	size_t free_amount;
	size_t free_no;
	size_t open_no;
	size_t close_no;
} at_track_stats_t;

typedef enum at_list_type
{
	AT_LIST_TYPE_HEAP,
	AT_LIST_TYPE_FILE
} at_list_type_t;

typedef struct at_list_item
{
	size_t id;
	struct at_list_item *prev;
	struct at_list_item *next;
	char *filename;
	char *function;
	int line;
} at_list_item_t;

typedef struct at_heap_list_item
{
	size_t id;
	struct at_heap_list_item *prev;
	struct at_heap_list_item *next;
	char *filename;
	char *function;
	int line;
	void *pointer;
	long size;
} at_heap_list_item_t;

typedef struct at_file_list_item
{
	size_t id;
	struct at_file_list_item *prev;
	struct at_file_list_item *next;
	char *filename;
	char *function;
	int line;
	FILE *handle;
	char *name;
	char *mode;
} at_file_list_item_t;

typedef struct at_list
{
	size_t length;
	struct at_list_item *first;
	struct at_list_item *last;
	at_list_type_t type;
} at_list_t;

char *at_truncate_back(const char *, int);
char *at_truncate_front(const char *, int);

#if defined AT_TRUNCATE_BACK && AT_TRUNCATE_BACK > 0
#define at_truncate(S, L) at_truncate_back((S), (L))
#else
#define at_truncate(S, L) at_truncate_front((S), (L))
#endif

at_heap_list_item_t *at_heap_list_item_new(const char *, const char *, int);
at_file_list_item_t *at_file_list_item_new(const char *, const char *, int);

void at_list_add(at_list_t *, at_list_item_t *, at_list_type_t);
void at_list_remove(at_list_t *, void *);
at_list_item_t *at_list_get(at_list_t *, void *);
void at_list_free_item(at_list_item_t **, at_list_type_t);
void at_list_free(at_list_t *);
size_t at_list_length(at_list_t *);

void at_free_all(void);
void at_report(void);

void *at_malloc(size_t, const char *, const char *, int);
void *at_calloc(size_t, size_t, const char *, const char *, int);
void *at_realloc(void *, size_t, const char *, const char *, int);

#if defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 500 \
 || defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200809L \
 || defined _BSC_SOURCE || defined _SVID_SOURCE
char *at_strdup(const char *, const char *, const char *, int);
#else
#ifndef strdup
char *strdup(const char *);
#endif
#endif

#if defined __USE_XOPEN2K8 || __GLIBC_USE(LIB_EXT2) \
 || defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200809L \
 || defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 700 \
 || defined _GNU_SOURCE
size_t at_getline(char **, size_t *, FILE *, const char *, const char *, int);
size_t at_getdelim(char **, size_t *, int, FILE *, const char *, const char *, int);
#endif

void at_free(void *);

FILE *at_fopen(const char *, const char *, const char *, const char *, int);
FILE *at_freopen(char *, const char *, FILE *, const char *, const char *, int);
FILE *at_tmpfile(const char *, const char *, int);

void at_fclose(FILE *);

#ifdef __cplusplus
}
#endif

#endif
