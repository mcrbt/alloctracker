/*
 * alloctracker - track dynamic memory allocations and open files
 * Copyright (C) 2019-2020 Daniel Haase
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at
 * https://www.boost.org/LICENSE_1_0.txt
 *
 * The project is inspired by code written by my colleague Jan Z.
 *
 * File:    alloctracker.c
 * Author:  Daniel Haase
 *
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alloctracker_intern.h"

static at_track_stats_t stats;
static at_list_t *heap_list = NULL;
static at_list_t *file_list = NULL;
static size_t heap_id_counter = 0;
static size_t file_id_counter = 0;
static char can_record = (char)0;
static char can_report = (char)0;

#if ( ! defined _XOPEN_SOURCE || _XOPEN_SOURCE < 500) \
 && ( ! defined _POSIX_C_SOURCE || _POSIX_C_SOURCE < 200809L) \
 && ( ! defined _BSC_SOURCE && ! defined _SVID_SOURCE) \
 && ( ! defined strdup)
char *strdup(const char *str)
{
	char *dup = NULL;
	if(!str) return NULL;
	dup = (char *)malloc((strlen(str) + 1) * sizeof(char));
	strcpy(dup, str);
	return dup;
}
#endif

static const char *at_basename(const char *filename)
{
	if(!filename || !strlen(filename)) return "";
	return (strrchr(filename, '/') ? strrchr(filename, '/') + 1 : filename);
}

char *at_truncate_back(const char *str, int len)
{
	char *trunc = NULL;

	if(!str || !strlen(str)) return strdup("");
	if((len < 0) || (strlen(str) <= (unsigned int)len)) return strdup(str);

	trunc = (char *)malloc((len + 1) * sizeof(char));
	trunc = strdup(str);
	strcpy(&trunc[(len - 3)], "...");
	trunc[len] = '\0';
	return trunc;
}

char *at_truncate_front(const char *str, int len)
{
	char *trunc = NULL;

	if(!str || !strlen(str)) return strdup("");
	if((len < 0) || (strlen(str) <= (unsigned int)len)) return strdup(str);

	trunc = (char *)malloc((len + 1) * sizeof(char));
	strcpy(trunc, "...");
	strcat(trunc, &str[(strlen(str) - len + 3)]);
	return trunc;
}

static void at_track_stats_init(void)
{
	if(can_record) return;
	memset(&stats, '\0', sizeof(at_track_stats_t));
	can_record = (char)1;

	if(!can_report)
	{
		if((heap_list && heap_list->length) ||
		   (file_list && file_list->length))
			 can_report = (char)1;
	 }
}

static void at_track_stats_aquire(at_list_item_t *item, at_list_type_t type)
{
	at_heap_list_item_t *heap_item = NULL;

	if(!item) return;
	if(!can_record) at_track_stats_init();

	if(type == AT_LIST_TYPE_HEAP)
	{
		heap_item = (at_heap_list_item_t *)item;
		stats.alloc_amount += heap_item->size;
		++(stats.alloc_no);
	}
	else if(type == AT_LIST_TYPE_FILE) ++(stats.open_no);
}

static void at_track_stats_release(at_list_item_t *item, at_list_type_t type)
{
	at_heap_list_item_t *heap_item = NULL;

	if(!item) return;
	if(!can_record) at_track_stats_init();

	if(type == AT_LIST_TYPE_HEAP)
	{
		heap_item = (at_heap_list_item_t *)item;
		stats.free_amount += heap_item->size;
		++(stats.free_no);
	}
	else if(type == AT_LIST_TYPE_FILE) ++(stats.close_no);
}

at_heap_list_item_t *at_heap_list_item_new(const char *filename, const char *function, int line)
{
	at_heap_list_item_t *item = NULL;

	item = (at_heap_list_item_t *)malloc(sizeof(at_heap_list_item_t));

	if(filename)
	{
		item->filename = (char *)malloc((strlen(filename) + 1) * sizeof(char));
		strcpy(item->filename, (char *)filename);
	}
	else item->filename = NULL;

	if(function)
	{
		item->function = (char *)malloc((strlen(function) + 1) * sizeof(char));
		strcpy(item->function, (char *)function);
	}
	else item->function = NULL;

	item->line = line;
	item->id = (heap_id_counter++);
	item->prev = item->next = NULL;
	item->pointer = NULL;
	item->size = 0;
	return item;
}

at_file_list_item_t *at_file_list_item_new(const char *filename, const char *function, int line)
{
	at_file_list_item_t *item = NULL;

	item = (at_file_list_item_t *)malloc(sizeof(at_file_list_item_t));

	if(filename)
	{
		item->filename = (char *)malloc((strlen(filename) + 1) * sizeof(char));
		strcpy(item->filename, (char *)filename);
	}
	else item->filename = NULL;

	if(function)
	{
		item->function = (char *)malloc((strlen(function) + 1) * sizeof(char));
		strcpy(item->function, (char *)function);
	}
	else item->function = NULL;

	item->line = line;
	item->id = (file_id_counter++);
	item->prev = item->next = NULL;
	item->handle = NULL;
	item->name = NULL;
	item->mode = NULL;
	return item;
}

void at_list_add(at_list_t *list, at_list_item_t *item, at_list_type_t type)
{
	at_list_item_t *tmp = NULL;

	if(!item) return;

	if(!list)
	{
		list = (at_list_t *)malloc(sizeof(at_list_t));
		list->first = list->last = item;
		list->length = 1;
		list->type = type;
	}
	else
	{
		if(!(list->length))
		{
			list->first = list->last = item;
			list->length = 1;
		}
		else
		{
			tmp = list->last;
			item->prev = tmp;
			tmp->next = item;
			list->last = item;
			++(list->length);
		}
	}

	if((type == AT_LIST_TYPE_HEAP) && !heap_list) heap_list = list;
	if((type == AT_LIST_TYPE_FILE) && !file_list) file_list = list;

	at_track_stats_aquire(item, type);
	can_report = (char)1;
}

at_list_item_t *at_list_get(at_list_t *list, void *pointer)
{
	at_list_item_t *item = NULL;

	if(!pointer || !list) return NULL;
	if(!(list->length)) return NULL;

	item = list->first;

	while(item)
	{
		if(((list->type == AT_LIST_TYPE_HEAP) &&
		    (((at_heap_list_item_t *)item)->pointer == pointer)) ||
		   ((list->type == AT_LIST_TYPE_FILE) &&
		    (((at_file_list_item_t *)item)->handle == (FILE *)pointer)))
			 return item;
		item = item->next;
	}

	return NULL;
}

void at_list_remove(at_list_t *list, void *pointer)
{
	at_list_item_t *item = NULL;

	if(!pointer || !list) return;
	if(!(list->length)) return;

	if((item = at_list_get(list, pointer)))
	{
		if(item->prev) item->prev->next = item->next;
		else list->first = item->next;
		at_track_stats_release(item, list->type);
		at_list_free_item(&item, list->type);
		--(list->length);
	}

	return;
}

void at_list_free_item(at_list_item_t **item, at_list_type_t type)
{
	at_heap_list_item_t **heap_item = NULL;
	at_file_list_item_t **file_item = NULL;

	if(!item) return;
	if(!(*item)) return;

	if(type == AT_LIST_TYPE_HEAP)
	{
		heap_item = (at_heap_list_item_t **)item;

		if(((signed long)(*heap_item)->size) <= 0)
		{
			fprintf(stderr,
				"[heap] invalid allocation size of %ld bytes detected\n",
				((signed long)((*heap_item)->size)));
		}
		else at_free_null((*heap_item)->pointer);
	}
	else if(type == AT_LIST_TYPE_FILE)
	{
		file_item = (at_file_list_item_t **)item;
		if(((*file_item)->handle)) fclose((*file_item)->handle);
		at_free_null((*file_item)->name);
		at_free_null((*file_item)->mode);
	}

	at_free_null((*item)->filename);
	at_free_null((*item)->function);
	at_free_null((*item));
}

void at_list_free(at_list_t *list)
{
	at_list_item_t *item = NULL, *tmp = NULL;

	if(!list) return;

	if((list->length))
	{
		item = list->first;

		while(item)
		{
			tmp = item->next;
			at_list_free_item(&item, list->type);
			item = tmp;
		}
	}

	at_free_null(list);
}

size_t at_list_length(at_list_t *list)
{
	if(!list) return (size_t)0;
	return list->length;
}

void at_free_all(void)
{
	at_list_free(heap_list);
	at_list_free(file_list);
	can_report = (char)0;
}

void at_report(void)
{
	at_heap_list_item_t *heap_item = NULL;
	at_file_list_item_t *file_item = NULL;
	size_t leaksum = 0, leaks = 0, open = 0;
	char *file = NULL, *source = NULL, *func = NULL;

	if(!can_report) return;
	if(!heap_list && !file_list) return;
	if(!(heap_list->length) && !(file_list->length)) return;

	fprintf(stderr, "\nALLOC TRACKER REPORT:\n\n");

	if(heap_list && (heap_list->length))
	{
		heap_item = (at_heap_list_item_t *)(heap_list->first);

		if(heap_item) fprintf(stderr, "unfreed memory:\n");

		while(heap_item)
		{
			leaksum += heap_item->size;
			++leaks;

			source = at_truncate(at_basename(heap_item->filename), 20);
			func = at_truncate(heap_item->function, 20);

			fprintf(stderr,
				"  %-18p  %6ld B  %20s:%-4d  %s%s\n", heap_item->pointer,
				heap_item->size, source, heap_item->line, func,
				(strlen(func) ? "()" : ""));

			heap_item = heap_item->next;
			at_free_null(source);
			at_free_null(func);
		}

		fprintf(stderr,
			"\n  overall %lu byte%s in %lu block%s unfreed\n\n",
			leaksum, ((leaksum == 1) ? "" : "s"), leaks,
			((leaks == 1) ? "" : "s"));
	}

	if(file_list && (file_list->length))
	{
		file_item = (at_file_list_item_t *)(file_list->first);

		if(file_item) fprintf(stderr, "unclosed files:\n");

		while(file_item)
		{
			++open;
			file = at_truncate(at_basename(file_item->name), 20);
			source = at_truncate(at_basename(file_item->filename), 20);
			func = at_truncate(file_item->function, 20);

			fprintf(stderr,
				"  %-18p  %-20s  %-3s  %20s:%-4d  %s%s\n", (void *)(file_item->handle),
				file, file_item->mode, source, file_item->line, func,
				(strlen(func) ? "()" : ""));

			file_item = file_item->next;
			at_free_null(file);
			at_free_null(source);
			at_free_null(func);
		}

		fprintf(stderr,
			"\n  overall %lu file%s unclosed\n\n",
			open, ((open == 1) ? "" : "s"));
	}

	if(!can_record) return;

	fprintf(stderr, "system resource summary:\n");
	fprintf(stderr, "  heap space allocated:  %lu bytes\n", stats.alloc_amount);
	fprintf(stderr, "  heap space freed:      %lu bytes\n", stats.free_amount);
	fprintf(stderr, "  allocations:           %lu\n", stats.alloc_no);
	fprintf(stderr, "  frees:                 %lu\n", stats.free_no);
	fprintf(stderr, "  files opened:          %lu\n", stats.open_no);
	fprintf(stderr, "  files closed:          %lu\n", stats.close_no);
	fprintf(stderr, "\n\n");
}

void *at_malloc(size_t length, const char *filename,
	const char *function, int line)
{
	at_heap_list_item_t *item = NULL;

	if(!length || (((signed long)length) < 0))
	{
		fprintf(stderr,
			"[heap] invalid allocation request for %ld bytes detected\n",
			((signed long)length));
		return NULL;
	}

	item = at_heap_list_item_new(filename, function, line);
	item->pointer = (void *)malloc(length);
	if(item->pointer) item->size = length;
	else item->size = (long)(-1);
	at_list_add(heap_list, (at_list_item_t *)item, AT_LIST_TYPE_HEAP);
	return item->pointer;
}

void *at_calloc(size_t blocks, size_t length, const char *filename,
	const char *function, int line)
{
	at_heap_list_item_t *item = NULL;

	if(!blocks || !length ||
		 (((signed long)blocks) < 0) ||
	   (((signed long)length) < 0))
	{
		fprintf(stderr,
			"[heap] invalid allocation request for %ld bytes detected\n",
			(((signed long)blocks) * ((signed long)length)));
		return NULL;
	}

	item = at_heap_list_item_new(filename, function, line);
	item->pointer = (void *)calloc(blocks, length);
	if(item->pointer) item->size = (blocks * length);
	else item->size = (long)(-1);
	at_list_add(heap_list, (at_list_item_t *)item, AT_LIST_TYPE_HEAP);
	return item->pointer;
}

void *at_realloc(void *ptr, size_t length, const char *filename,
	const char *function, int line)
{
	at_heap_list_item_t *item = NULL;

	if(!ptr) return at_malloc(length, filename, function, line);

	if(((signed long)length) <= 0)
	{
		fprintf(stderr,
			"[heap] invalid allocation request for %ld bytes detected\n",
			((signed long)length));
	}

	item = (at_heap_list_item_t *)at_list_get(heap_list, ptr);
	if(!item) return NULL;
	item->pointer = (void *)realloc(ptr, length);
	if(item->pointer) item->size = length;
	else item->size = (long)(-1);

	if(strcmp(item->filename, filename))
	{
		at_free_null(item->filename);
		item->filename = (char *)malloc((strlen(filename) + 1) * sizeof(char));
		strcpy(item->filename, filename);
	}

	if(strcmp(item->function, function))
	{
		at_free_null(item->function);
		item->function = (char *)malloc((strlen(function) + 1) * sizeof(char));
		strcpy(item->function, function);
	}

	item->line = line;

	return item->pointer;
}

#if defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 500 \
 || defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200809L \
 || defined _BSC_SOURCE || defined _SVID_SOURCE
char *at_strdup(const char *string, const char *filename,
	const char *function, int line)
{
	at_heap_list_item_t *item = NULL;
	char *pointer = NULL;

	if(!string) return NULL;
	if(!strlen(string)) return NULL;
	item = at_heap_list_item_new(filename, function, line);
	pointer = (char *)malloc((strlen(string) + 1) * sizeof(char));

	if(pointer)
	{
		strcpy(pointer, string);
		item->size = (strlen(pointer) + 1);
	}

	item->pointer = pointer;
	at_list_add(heap_list, (at_list_item_t *)item, AT_LIST_TYPE_HEAP);
	return pointer;
}
#endif

#if defined __USE_XOPEN2K8 || __GLIBC_USE(LIB_EXT2) \
 || defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200809L \
 || defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 700 \
 || defined _GNU_SOURCE
size_t at_getline(char **outline, size_t *buflen, FILE *stream,
	const char *filename, const char *function, int line)
{
	return at_getdelim(outline, buflen, '\n', stream,
	                   filename, function, line);
}

size_t at_getdelim(char **outline, size_t *buflen, int delim, FILE *stream,
	const char *filename, const char *function, int line)
{
	at_heap_list_item_t *item = NULL;
	size_t nbuflen = *buflen, linelen = 0;
	char *preallocated = NULL;

	if(!outline || !buflen || !stream) return (size_t)(-1);

	if(!(*outline)/* && !(*buflen)*/)
	{
		item = at_heap_list_item_new(filename, function, line);
		if(delim == '\n') linelen = getline(outline, buflen, stream);
		else linelen = getdelim(outline, buflen, delim, stream);

    if((linelen == (size_t)(-1)) &&
       ((errno == EINVAL) || (errno == ENOMEM)))
      return linelen;

		item->size = *buflen;
		item->pointer = *outline;
		at_list_add(heap_list, (at_list_item_t *)item, AT_LIST_TYPE_HEAP);
	}
	else
	{
		preallocated = *outline;

		if(delim == '\n') linelen = getline(outline, &nbuflen, stream);
		else linelen = getdelim(outline, &nbuflen, delim, stream);

    if((linelen == (size_t)(-1)) &&
       ((errno == EINVAL) || (errno == ENOMEM)))
      return linelen;

		if(nbuflen > *buflen)
		{
			item = (at_heap_list_item_t *)at_list_get(heap_list, (void *)(*outline));

			if(item) item->size = nbuflen;
			else
			{
				item = (at_heap_list_item_t *)at_list_get(heap_list, (void *)preallocated);

				if(item) /* ((preallocated != *outline) == true) implicitly holds */
				{
					item->pointer = *outline;
					item->size = nbuflen;
          /* "preallocated" has already been freed by "realloc"
             inside "getline", or "getdelim", respectively */
				}
				/* else: buffer is not allocated on heap but on stack */
			}

			*buflen = nbuflen;
		}
	}

	return linelen;
}
#endif

void at_free(void *pointer)
{
	if(!pointer) return;
	at_list_remove(heap_list, pointer);
}

FILE *at_fopen(const char *name, const char *mode, const char *filename,
	const char *function, int line)
{
	at_file_list_item_t *item = NULL;

	if(!name || !strlen(name)) return NULL;
	if(!mode || !strlen(mode)) return NULL;

	item = at_file_list_item_new(filename, function, line);
	item->handle = fopen(name, mode);
	item->name = (char *)malloc((strlen(name) + 1) * sizeof(char));
	strcpy(item->name, name);
	item->mode = (char *)malloc((strlen(mode) + 1) * sizeof(char));
	strcpy(item->mode, mode);
	at_list_add(file_list, (at_list_item_t *)item, AT_LIST_TYPE_FILE);
	return item->handle;
}

FILE *at_freopen(char *name, const char *mode, FILE *stream,
	const char *filename, const char *function, int line)
{
	at_file_list_item_t *item = NULL;
	FILE *tmphandle = NULL;

	if(!mode || !strlen(mode)) return NULL;
	if(!stream) return NULL;

	item = (at_file_list_item_t *)at_list_get(file_list, (void *)stream);

	if(item)
	{
		if(!name) name = item->name;

		if(item->name && !strcmp(item->name, name))
		{
			if(strcmp(item->mode, mode))
			{
				at_free_null(item->mode);
				item->mode = (char *)malloc((strlen(mode) + 1) * sizeof(char));
				strcpy(item->mode, mode);
			}
		}
		else
		{
			at_list_remove(file_list, (void *)stream);
			item = NULL;
		}
	}

	if(!item)
	{
		item = at_file_list_item_new(filename, function, line);

		if(name)
		{
			item->name = (char *)malloc((strlen(name) + 1) * sizeof(char));
			strcpy(item->name, name);
		}
		else item->name = NULL;

		item->mode = (char *)malloc((strlen(mode) + 1) * sizeof(char));
		strcpy(item->mode, mode);

		if((item->handle = freopen(name, mode, stream)))
			at_list_add(file_list, (at_list_item_t *)item, AT_LIST_TYPE_FILE);
		else
		{
			at_track_stats_release((at_list_item_t *)item, AT_LIST_TYPE_FILE);
			at_list_free_item((at_list_item_t **)&item, AT_LIST_TYPE_FILE);
		}
	}
	else
	{
		tmphandle = freopen(name, mode, stream);

		if(tmphandle && (tmphandle != (item->handle)))
		{
			at_list_remove(file_list, (void *)(item->handle));

			item = at_file_list_item_new(filename, function, line);

			if(name)
			{
				item->name = (char *)malloc((strlen(name) + 1) * sizeof(char));
				strcpy(item->name, name);
			}
			else item->name = NULL;

			item->mode = (char *)malloc((strlen(mode) + 1) * sizeof(char));
			strcpy(item->mode, mode);

			item->handle = tmphandle;

			at_list_add(file_list, (at_list_item_t *)item, AT_LIST_TYPE_FILE);
		}
		else
		{
			if(!tmphandle)
			{
				if(item->handle)
					at_list_remove(file_list, (void *)(item->handle));
				else
				{
					at_track_stats_release((at_list_item_t *)item, AT_LIST_TYPE_FILE);
					at_list_free_item((at_list_item_t **)&item, AT_LIST_TYPE_FILE);
				}
			}
			else item->handle = tmphandle;
		}
	}

	return item->handle;
}

FILE *at_tmpfile(const char *filename, const char *function, int line)
{
	at_file_list_item_t *item = at_file_list_item_new(filename, function, line);

	if(!item) return NULL;

	item->handle = tmpfile();
	item->name = NULL;
	item->mode = strdup("wb+");
	at_list_add(file_list, (at_list_item_t *)item, AT_LIST_TYPE_FILE);

	return item->handle;
}

void at_fclose(FILE *file)
{
	if(!file) return;
	at_list_remove(file_list, (void *)file);
}

char *at_version(void) { return ALLOC_TRACKER_VERSION; }
