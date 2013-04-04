// Demo1.c - demonstrates signal handling

// compile from the command line with "gcc -Wall demo1.c"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <unistd.h>


/* This is the signal handler */
void handler(int signal) {
	/* Reinstall the timer to alarm in 3 secs. */
	alarm(3);
	printf("in signal handler\n");
}

int main(void) {
	struct sigaction sa;

	/* Initialize the data structures for signal handling. */
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);
	sa.sa_handler = handler;

	/* Install the signal handler */
	if (sigaction(SIGALRM, &sa, NULL) < 0)
		abort();

	/* Install the timer to alrm in 3 secs. */
	alarm(3);

	/* Loop forever */
	while(1);

	return 0;
}

