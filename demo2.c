// Demo2.c - demonstrates non local branching
//           creates 2 threads and swaps from one to another

// compile from the command line with "gcc -Wall demo2.c"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <unistd.h>

#define THREAD_NUM 2

/* Set by the signal handler. */
static volatile int currThreadNum;
;

/* The contexts. */
static ucontext_t uc[THREAD_NUM + 1];

/* This is the function doing the work. */
static void f(int n) {
	while (1) {

		printf("in thread #%d\n", n);

		sleep(1);

	}
}

/* This is the signal handler which swaps between the threads. */
void handler(int signal) {
	alarm(3);
	currThreadNum = (currThreadNum % THREAD_NUM) + 1;
	printf("in signal handler: switching from %d to %d\n", currThreadNum, THREAD_NUM + 1
			- currThreadNum);
	swapcontext(&uc[currThreadNum], &uc[THREAD_NUM + 1 - currThreadNum]);

}

int main(void) {
	struct sigaction sa;

	char st1[8192];
	char st2[8192];

	/* Initialize the data structures for SIGALRM handling. */
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);
	sa.sa_handler = handler;

	/* Install the signal handler and get the context we can manipulate. */
	if (sigaction(SIGALRM, &sa, NULL) < 0 || getcontext(&uc[1]) == -1
			|| getcontext(&uc[2]) == -1)
		abort();

	/* Create a context with a separate stack which causes the
	 function f to be call with the parameter 1.
	 Note that the uc_link points to the main context
	 which will cause the program to terminate once the function
	 return. */
	uc[1].uc_link = &uc[0];
	uc[1].uc_stack.ss_sp = st1;
	uc[1].uc_stack.ss_size = sizeof st1;
	makecontext(&uc[1], (void(*)(void)) f, 1, 1);

	/* Similarly, but 2 is passed as the parameter to f. */
	uc[2].uc_link = &uc[0];
	uc[2].uc_stack.ss_sp = st2;
	uc[2].uc_stack.ss_size = sizeof st2;
	makecontext(&uc[2], (void(*)(void)) f, 1, 2);

	/* Start running. */
	alarm(3);
	currThreadNum = 0;
	swapcontext(&uc[0], &uc[1]);

	return 0;
}

