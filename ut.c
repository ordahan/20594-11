/*
 * ut.c
 *
 *  Created on: Apr 5, 2013
 *      Author: Or Dahan 201644929
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "ut.h"

/* Internal definitions */
const unsigned int QUANTOM_SEC = 1;

typedef void (*thread_main)(int);

/* Global structures */
static ucontext_t s_runner_context; /* Context of the thread that started the library */
static ut_slot s_threads;
static unsigned int s_threads_size = 0;
static unsigned int s_num_spawned_threads = 0;
static tid_t s_running_thread_id = 0;
static ucontext_t s_current_context;

/* Internal functions */

/**
 * Sets the number of threads in the inner table.
 * If the given size isn't in the limits
 * given by MAX & MIN TAB_SIZE, sets the size
 * to MAX.
 * @param tab_size Num of threads wanted
 */
void set_num_threads_in_valid_range(int tab_size);

/**
 * Swap the current running thread with the next one
 * (circular) on the list in a round-robin fashion.
 * @param signal Signal type that caused the scheduler
 * to run.
 */
void scheduler(int signal);

/* Implementations */
void set_num_threads_in_valid_range(int tab_size)
{
	/* Illegal table size */
	if (tab_size < MIN_TAB_SIZE ||
		tab_size > MAX_TAB_SIZE||
		tab_size < 0) /* As this is a signed integer, it might be negative
					which of-course has no real meaning.
					Assume MIN_num is >= 0? not healthy.*/
	{
		s_threads_size = MAX_TAB_SIZE;
	}
	/* Legal size */
	else
	{
		s_threads_size = tab_size;
	}
}


int ut_init(int tab_size)
{
	/* Init counters */
	s_num_spawned_threads = 0;
	s_threads_size = 0;

	/* Delete the current table if exists */
	if (s_threads != NULL)
	{
		free(s_threads);
	}

	/* Set the table size */
	set_num_threads_in_valid_range(tab_size);

	/* Allocate the threads table */
	s_threads = malloc(sizeof(s_threads[0]) * s_threads_size);

	/* Make sure the allocation was successful */
	if (s_threads == NULL)
	{
		return SYS_ERR;
	}

	return 0;
}


tid_t ut_spawn_thread(thread_main main, int arg)
{
	unsigned int current_thread_num = s_num_spawned_threads;
	ut_slot pCurrThreadSlot = &s_threads[current_thread_num];
	ucontext_t* pCurrThreadContext = &pCurrThreadSlot->uc;

	/* Assert that the lib was initiated already */
	if (s_threads == NULL)
	{
		return SYS_ERR;
	}

	/* Make sure there is room in the table */
	if (s_num_spawned_threads >= s_threads_size)
	{
		return TAB_FULL;
	}
	/* Count the new thread as created */
	else
	{
		s_num_spawned_threads++;
	}

	/* Get the context of the current thread */
	if (getcontext(pCurrThreadContext) == SYS_ERR)
	{
		return SYS_ERR;
	}

	/* Create the proper context of the new thread
	 * derived from the current one,
	 * link it to the running context.
	 */
	pCurrThreadContext->uc_link = &s_runner_context;

	/* Allocate stack space for the thread */
	pCurrThreadContext->uc_stack.ss_sp = malloc(STACKSIZE);
	pCurrThreadContext->uc_stack.ss_size = STACKSIZE;

	/* Make sure stack allocated correctly */
	if (pCurrThreadContext->uc_stack.ss_sp == NULL)
	{
		return SYS_ERR;
	}

	/* Set the thread's function and arg */
	pCurrThreadSlot->func = main;
	pCurrThreadSlot->arg = arg;

	/* Init the running time */
	pCurrThreadSlot->vtime = 0;

	return current_thread_num;
}

int ut_start(void)
{
	struct sigaction sa;
	unsigned int i;

	/* Set the 'scheduler' signal */
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);
	sa.sa_handler = scheduler;
	if (sigaction(SIGALRM, &sa, NULL) < 0)
	{
		return SYS_ERR;
	}

	/* Set the 'profiler' signal */

	/* Start all the threads */
	for (i = 0; i < s_num_spawned_threads; ++i)
	{
		ut_slot pCurrThread = &s_threads[i];

		/* Start the current thread */
		errno = 0;
		makecontext(&pCurrThread->uc,
					(void(*)(void))pCurrThread->func,
					1, /* Single argument */
					pCurrThread->arg);

		/* Make sure it started correctly */
		if (errno != 0)
		{
			return SYS_ERR;
		}
	}

	/* Init to run the first thread */
	s_running_thread_id = 0;

	/* Start running the system */

	alarm(QUANTOM_SEC);
	swapcontext(&s_runner_context, &s_threads[s_running_thread_id].uc);

	return 0;
}

unsigned long ut_get_vtime(tid_t tid)
{
	return 0;
}

void scheduler(int signal)
{
	/* Set the next scheduling to occur */
	alarm(QUANTOM_SEC);

	/* Handle thread list's circularity,
	 * next one on the list */
	s_running_thread_id = (s_running_thread_id + 1) % s_threads_size;
/*	printf("in signal handler: switching from %d to %d\n",
			s_running_thread,
			THREAD_NUM + 1 - s_running_thread); */
	swapcontext(&s_current_context, &s_threads[s_running_thread_id].uc);
}
