// Demo3.c - demonstrates virtual timers


// compile from the command line with "gcc -Wall demo3.c"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <unistd.h>

static unsigned long vtime;

/* This is the signal handler which swaps between the threads. */
void handler(int signal) {

	if (signal == SIGVTALRM) {
		// update the vtime statistics
		vtime += 10;

	} else if (signal == SIGINT) { // if CTRL-C is pressed print the statistics and terminate
		printf("\nvtime %ld.%ld sec.\n", vtime / 1000, vtime % 1000);
		exit(0);
	}

}

int main(void) {

	struct sigaction sa;
	struct itimerval itv;

	/* Initialize the data structures for SIGALRM handling. */
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);
	sa.sa_handler = handler;

	/* set up vtimer for accounting */
	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 10000;
	itv.it_value = itv.it_interval;

	printf("Press CTRL-C to print the statistics and terminate\n");

	if (sigaction(SIGVTALRM, &sa, NULL) < 0)
		abort();

	if (setitimer(ITIMER_VIRTUAL, &itv, NULL) < 0)
		abort();

	if (sigaction(SIGINT, &sa, NULL) < 0)
		abort();

	while (1)
		;
	return 0;
}

