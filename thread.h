#include <pthread.h>
#include <stdlib.h>
#include "stack.h" // stack super coolness
#include "gerr.h"

// Include guard.
#ifndef THREAD
#define THREAD

// Super simple threads interface

typedef void *(*proc) (void *v);

int pspawn (Stack *stack, proc func, void *arg) {
	pthread_t *th = malloc(sizeof (pthread_t));
	int err = pthread_create(th, NULL, func, arg);
	if (err != 0){return 1;}
	push(stack, (void *) th); // push to stack
	return 0;
}

// wait for all processes
int pwait (Stack *thread) {
	pthread_t *th = (pthread_t *) pop(thread);
	int err = pthread_join(*th, NULL);
	if (err != 0){return 1;}
	free(th);
	return 0;
}

#endif // THREAD