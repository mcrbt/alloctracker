/**
 * alloctracker - track dynamic memory allocations and open files
 * Copyright (C) 2019-2020 Daniel Haase
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at
 * https://www.boost.org/LICENSE_1_0.txt
 *
 * The project is inspired by code written by my colleague Jan Z.
 *
 * File:    at_test.c
 * Author:  Daniel Haase
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alloctracker.h"

void exit_handler(void)
{
	remove("rsc/nosuchfilewithaverylongname.dat");
	AT_REPORT;
	AT_FREE_ALL;
	printf("[ ok ] done\n\n");
}

static void test_malloc(void)
{
	char *string = (char *)malloc(1024);
	char **memory = (char **)malloc(3 * sizeof(char *));
	int i;

	strcpy(string, "hello world\n");

	for(i = 0; i < 3; i++)
	{
		memory[i] = (char *)malloc(64 * sizeof(char));
		sprintf(memory[i], "string %d", (i + 1));
	}
}

static void test_calloc(void)
{
	char *memory = (char *)calloc(4, sizeof(char));
	int i;

	for(i = 0; i < 4; i++)
	{
		if(memory[i] != '\0')
			printf("[warn] memory[%d] is not initialized", i);
	}
}

static void test_realloc(void)
{
	char *memory1 = (char *)malloc(2048 * sizeof(char));
	char *memory2 = (char *)malloc(2 * sizeof(char));

	memory1 = (char *)realloc(memory1, 256);
	memory2 = (char *)realloc(memory2, 128);
}

#if defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 500 \
 || defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200809L \
 || defined _BSC_SOURCE || defined _SVID_SOURCE
static void test_strdup(void)
{
	char *alphabet = strdup("abcdefghijklmnopqrstuvwxyz");
	assert(strlen(alphabet) == 26);
}
#endif

#if defined __USE_XOPEN2K8 || __GLIBC_USE(LIB_EXT2) \
 || defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200809L \
 || defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 700 \
 || defined _GNU_SOURCE
static void test_getline(const char prealloc)
{
	FILE *file = NULL;
	char *filename = "rsc/getline.txt";
	char *line = NULL;
	size_t blen = 0;
	long len = 0;

	if(prealloc)
	{
		blen = 16;
		line = (char *)malloc(blen * sizeof(char));
		strcpy(line, "\0");
	}

	if(!(file = fopen(filename, "r")))
	{
		fprintf(stderr,
			"[erro] failed to open file \"%s\" for reading\n",
			filename);
		return;
	}

	while(!feof(file))
	{
		if((len = (long)getline(&line, &blen, file)) == (-1L))
			break; /* error or end of file condition */

		if(line)
		{
			if((len > (-1L)) && (line[(strlen(line) - 1)] == '\n'))
					line[(strlen(line) - 1)] = '\0';
		}
		else len = 0;

		assert(len < (signed long)blen);

		if(len)
		{
			printf("[info] read %4ld bytes into buffer of %4lu bytes\n", len, blen);
			/* printf("         \"%s\"\n", (line ? line : "")); */
		}

		if(!feof(file))
		{
			if(prealloc)
			{
				blen = 16;
				line = (char *)malloc(blen * sizeof(char));
				strcpy(line, "\0");
			}
			else
			{
				blen = 0;
				line = NULL;
			}
		}
	}

	fclose(file);
	printf("\n");
}
#endif

static void test_fopen(void)
{
	FILE *exis = fopen("rsc/lines.txt", "r");
	FILE *away = fopen("rsc/nosuchfilewithaverylongname.dat", "wb");
	assert(exis);
	assert(away);
}

static void test_freopen(void)
{
	FILE *open = fopen("rsc/lines.txt", "w");
	FILE *reop1 = freopen("rsc/lines.txt", "r", open);
	FILE *reop2 = freopen(NULL, "a+", reop1);
	assert(reop2);
}

static void test_tmpfile(void)
{
	FILE *temp1 = tmpfile();
	FILE *temp2 = tmpfile();
	assert(temp1);
	assert(temp2);
}

int main(void)
{
	atexit(exit_handler);

	printf("\n[ ok ] alloctracker version %s\n", at_version());

#ifdef AT_ALLOC_TRACK
	printf("[info] AT_ALLOC_TRACK defined\n\n");
#else
	printf("[info] AT_ALLOC_TRACK not defined\n\n");
#endif

	test_malloc();
	test_calloc();
	test_realloc();

#if defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 500 \
 || defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200809L \
 || defined _BSC_SOURCE || defined _SVID_SOURCE
 test_strdup();
#endif

#if defined __USE_XOPEN2K8 || __GLIBC_USE(LIB_EXT2) \
 || defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200809L \
 || defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 700 \
 || defined _GNU_SOURCE
	test_getline((char)0); /* let "getline" allocate buffer */
	test_getline((char)1); /* preallocate buffer for "getline" */
#endif

	test_fopen();
	test_freopen();
	test_tmpfile();

	return 0;
}
