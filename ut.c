/*
 * ut.c
 *
 *  Created on: Apr 5, 2013
 *      Author: Or Dahan 201644929
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "ut.h"

/* Internal definitions */
#define QUANTOM_SEC (1)
#define PROFILER_INTERVAL_MSEC (10)
#define PROFILER_INTERVAL_USEC (PROFILER_INTERVAL_MSEC * 1000)

typedef void (*thread_main)(int);

/* Global structures */
static ut_slot s_threads;
static unsigned int s_threads_size = 0;
static unsigned int s_num_spawned_threads = 0;
static tid_t s_current_thread_id = 0;

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

/**
 * Counts the number of msec the current thread is running.
 * @param signal
 */
void profiler(int signal);

/**
 * Initializes the scheduler mechanism.
 * @return 0 - Success
 * 		   SYS_ERR - On any failure
 */
unsigned int init_scheduler();

/**
 * Initializes the profiler mechanism.
 * @return 0 - Success
 * 		   SYS_ERR - On any failure
 */
int init_profiler();

/**
 * Prepares all the threads in the table
 * to "ready" mode
 * @return 0 - Success
 		   SYS_ERR - On any failure
 */
int prepare_all_threads();

/* Implementations */
void set_num_threads_in_valid_range(int tab_size)
{
	/* Illegal table size */
	if (tab_size < MIN_TAB_SIZE ||
		tab_size > MAX_TAB_SIZE||
		tab_size < 0) /* As this is a signed integer, it might be negative
					which of-course has no real meaning.
					Assume MIN_TAB_SIZE is >= 0? not healthy.*/
	{
		s_threads_size = MAX_TAB_SIZE;
	}
	/* Legal size */
	else
	{
		/* Save room for the original dummy thread at 0*/
		s_threads_size = tab_size + 1;
	}
}

int ut_init(int tab_size)
{
	/* Init counters */
	s_num_spawned_threads = 0;
	s_threads_size = 0;

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
	unsigned int current_slot = s_num_spawned_threads + 1;
	ut_slot pCurrThreadSlot = &s_threads[current_slot];
	ucontext_t* pCurrThreadContext = &pCurrThreadSlot->uc;

	/* Assert that the lib was initiated already */
	if (s_threads == NULL)
	{
		return SYS_ERR;
	}

	/* Make sure there is room in the table */
	if (current_slot >= s_threads_size)
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
	 * link it to original thread.
	 */
	pCurrThreadContext->uc_link = &s_threads[0].uc;

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

	/* The thread at slot 0 is the main thread,
	 * we don't count it as a thread with TID,
	 * so slot 1 is TID 0 de-facto.
	 */
	return current_slot - 1;
}

int ut_start(void)
{
	/* Init scheduler mechanism */
	if (init_scheduler() != 0) return SYS_ERR;

	/* Set the 'profiler' signal */
	if (init_profiler() != 0) return SYS_ERR;

	/* Start all the threads */
	if (prepare_all_threads() != 0) return SYS_ERR;

	/* Init to run the first thread */
	s_current_thread_id = 0;

	/* Start running the system */
	errno = 0;
	alarm(QUANTOM_SEC);

	/* If swapcontext returns, its an error.
	 * In that case 'errno' will be set on any failure.
	 */
	swapcontext(&s_threads[0].uc,
				&s_threads[1].uc);

	/* Make sure system started */
	if (errno != 0)
	{
		return SYS_ERR;
	}

	return 0;
}

unsigned long ut_get_vtime(tid_t tid)
{
	/* Don't access illegal memory */
	if (s_threads == NULL ||
		tid > s_threads_size)
	{
		return 0;
	}

	return s_threads[tid].vtime;
}

void scheduler(int signal)
{
	tid_t previous_thread_id = s_current_thread_id;

	// Make sure thread table allocated
	if (s_threads == NULL)
	{
		errno = 1;
		perror("scheduler cannot access thread table as it doesn't exist.\n");
		exit(1);
	}

	/* Set the next scheduling to occur */
	errno = 0;
	alarm(QUANTOM_SEC);

	/* Handle list's circularity, next thread */
	s_current_thread_id++;
	if (s_current_thread_id == s_num_spawned_threads)
	{
		s_current_thread_id = 0;
	}

	/* Swap to the current one */
	swapcontext(&s_threads[previous_thread_id + 1].uc,
			    &s_threads[s_current_thread_id + 1].uc);

	/* Critical error.. */
	if (errno != 0)
	{
		perror("scheduler\n");
		exit(1);
	}
}

unsigned int init_scheduler()
{
	struct sigaction sa;

	/* Prepare the scheduler's handler struct */
	sa.sa_flags = SA_RESTART;
	if (sigfillset(&sa.sa_mask) == SYS_ERR) return SYS_ERR;
	sa.sa_handler = scheduler;

	/* Set the 'scheduler' signal */
	if (sigaction(SIGALRM, &sa, NULL) < 0) return SYS_ERR;

	return 0;
}

void profiler(int signal)
{
	// Make sure thread table allocated
	if (s_threads == NULL)
	{
		errno = 1;
		perror("profiler cannot access thread table as it doesn't exist.\n");
		exit(1);
	}

	// Update the current running thread's run counter
	s_threads[s_current_thread_id].vtime += PROFILER_INTERVAL_MSEC;
}

int init_profiler()
{
	struct itimerval itv;
	struct sigaction sa;

	/* Initialize the data structures for SIGVTALRM handling. */
	sa.sa_flags = SA_RESTART;
	if (sigfillset(&sa.sa_mask) == SYS_ERR) return SYS_ERR;
	sa.sa_handler = profiler;

	/* set up vtimer for accounting */
	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = PROFILER_INTERVAL_USEC;
	itv.it_value = itv.it_interval;

	/* Set the profiler signal */
	if (sigaction(SIGVTALRM, &sa, NULL) < 0)
	{
		return SYS_ERR;
	}

	/* Start the profiler timer */
	if (setitimer(ITIMER_VIRTUAL, &itv, NULL) < 0)
	{
		return SYS_ERR;
	}

	return 0;
}

int prepare_all_threads()
{
	unsigned int i;

	/* Go thread by thread */
	for (i = 1; i <= s_num_spawned_threads; ++i)
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

	return 0;
}
