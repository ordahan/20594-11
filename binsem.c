/*
 * binsem.c
 *
 *  Created on: Apr 5, 2013
 *      Author: Or Dahan 201644929
 */

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "binsem.h"

void binsem_init(sem_t *s, int init_val)
{
	/* Must be a valid semaphore */
	if (s == NULL) return;

	/* Anything other than 0 is 1 */
	if (init_val == 0)
	{
		*s = 0;
	}
	else
	{
		*s = 1;
	}
}

void binsem_up(sem_t *s)
{
	/* Must be a valid semaphore */
	if (s == NULL) return;

	/* Doesn't matter if it was 0 or 1, now
	 * its 1 either way (waiting threads
	 * will be released)
	 */
	*s = 1;
}

int binsem_down(sem_t *s)
{
	/* Must be a valid semaphore */
	if (s == NULL) return 0;

	/* Try acquiring the semaphore */
	while (xchg(s, 0) == 0)
	{
		/* Don't belong to us,
		 * signal the scheduler to
		 * run another thread
		 */
		kill(getpid(), SIGALRM);

		/* Make sure we completed ok */
		if (errno != 0)
		{
			return -1;
		}
	}

	return 0;
}
