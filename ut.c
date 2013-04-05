/*
 * ut.c
 *
 *  Created on: Apr 5, 2013
 *      Author: Or Dahan 201644929
 */

#include <stdlib.h>

#include "ut.h"

/* Internal types */
typedef void (*thread_main)(int);

/* Global structures */
static ut_slot* s_threads;
unsigned int s_num_threads = 0;

/* Internal functions */

/**
 * Sets the number of threads in the inner table.
 * If the given size isn't in the limits
 * given by MAX & MIN TAB_SIZE, sets the size
 * to MAX.
 * @param num Num of threads wanted
 */
void set_num_threads_in_valid_range(int num);

/* Implementations */
void set_num_threads_in_valid_range(int num)
{
	/* Illegal table size */
	if (num < MIN_TAB_SIZE ||
		num > MAX_TAB_SIZE||
		num < 0) /* As this is a signed integer, it might be negative
					which of-course has no real meaning.
					Assume MIN_num is >= 0? not healthy.*/
	{
		s_num_threads = MAX_TAB_SIZE;
	}
	/* Legal size */
	else
	{
		s_num_threads = num;
	}
}


int ut_init(int tab_size)
{
	/* Set the table size */
	set_num_threads_in_valid_range(tab_size);

	/* Allocate the threads table */
	s_threads = malloc(sizeof(s_threads[0]) * s_num_threads);

	/* Make sure the allocation was successful */
	if (s_threads == NULL)
	{
		return SYS_ERR;
	}

	return 0;
}


tid_t ut_spawn_thread(void (*func)(int), int arg)
{
	return 0;
}

int ut_start(void)
{
	return 0;
}

unsigned long ut_get_vtime(tid_t tid)
{
	return 0;
}
