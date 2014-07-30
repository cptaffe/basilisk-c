#import <pthread.h>

// Simple functions for concurrently working with recourses
// when using pthreads & Basilisk

// Include guard.
#ifndef CONCURRENT
#define CONCURRENT

// Mutex Stack
typedef struct {
	void **stack; // stack of void pointers.
	int len; // length of stack
	int max;
	int index; // read from base length
	pthread_mutex_t *lock;
	pthread_cond_t *cond;
} MutexStack;

MutexStack *initmstack() {
	// allocate ErrorStack on heap memory
	MutexStack *stack = (MutexStack *) malloc(sizeof (MutexStack));
	if (stack == NULL){return NULL;}

	stack->stack = NULL;
	stack->len = 0;
	stack->max = 0;
	stack->index = 0;

	stack->lock = malloc(sizeof (pthread_mutex_t));
	if (stack->lock == NULL){return NULL;}

	stack->cond = malloc(sizeof (pthread_cond_t));
	if (stack->cond == NULL){return NULL;}

	pthread_cond_init(stack->cond, NULL);
	pthread_mutex_init(stack->lock, NULL);
	return stack;
}

int freemstack(MutexStack *stack) {
	pthread_mutex_destroy(stack->lock);
	free(stack->stack);
	free(stack);
	return 0;
}

int resizemstack (MutexStack *stack) {
	int grow, shrink;
	grow = stack == NULL || stack->len >= stack->max;
	shrink = (stack->max - stack->len) > StackBuf;
	if (!grow && !shrink) {return 0;}
	
	// reallocate len+buf # of Error pointer positions
	int buff;
	if (grow){buff = StackBuf;}
	else{buff = 0;}
	size_t size = (stack->len + buff) * sizeof (void *);
	stack->stack = (void **) realloc(stack->stack, size);
	if (stack->stack == NULL){free(stack->stack); return 1;}
	stack->max = size / sizeof (void *); // new max is len
	return 0;
}

// Should be called from thread not calling mpush
void *mnext (MutexStack *stack) {
	pthread_mutex_lock(stack->lock);
	if (stack->index >= stack->len){
		pthread_cond_wait(stack->cond, stack->lock);
	}
	void *v = stack->stack[stack->index];
	stack->index++;
	pthread_mutex_unlock(stack->lock);
	return v;
}

// Should be called from thread not calling mpush
int mbackup (MutexStack *stack) {
	pthread_mutex_lock(stack->lock);
	stack->index--;
	pthread_mutex_unlock(stack->lock);
	return stack->index;
}

// pops error from an error stack
void *mpop (MutexStack *stack) {
	pthread_mutex_lock(stack->lock);
	if (stack->len <= 0){
		pthread_cond_wait(stack->cond, stack->lock);
	} // wait on length to be long enough

	// resize stack
	if(resizemstack(stack)){return NULL;}

	stack->len--; // decrement len
	if (stack->len < 0){return NULL;}

	pthread_mutex_unlock(stack->lock);
	return stack->stack[stack->len]; // pass reference to Error
}

// pushes error onto an error stack
int mpush (MutexStack *stack, void *v) {
	pthread_mutex_lock(stack->lock);
	pthread_cond_broadcast(stack->cond);
	// resize stack
	if(resizemstack(stack)){return 1;}

	stack->len++; // increment len
	if (stack->len <= 0){return 1;}

	if (v == NULL) {return 1;}

	// Push to created space on stack
	stack->stack[stack->len - 1] = v;
	pthread_mutex_unlock(stack->lock);
	return 0;
}

// "dumps" all values
int resetmstack(MutexStack *stack) {
	pthread_mutex_lock(stack->lock);
	stack->len = 0; // zero errors in stack
	pthread_mutex_unlock(stack->lock);
	return 0;
}

#endif // CONCURRENT