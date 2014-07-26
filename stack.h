// Consolidated stack management for any type.

// Include guard.
#ifndef STACK
#define STACK

// Stack
typedef struct {
	void **stack; // stack of void pointers.
	int len; // length of stack
	int max;
} Stack;

const int StackBuf = 5;

Stack *initstack() {
	// allocate ErrorStack on heap memory
	Stack *stack = (Stack *) malloc(sizeof (Stack));
	if (stack == NULL){return NULL;}

	stack->stack = NULL;
	stack->len = 0;
	stack->max = 0;
	return stack;
}

int freestack(Stack *stack) {
	free(stack->stack);
	free(stack);
	return 0;
}

int resizestack (Stack *stack) {
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

// pops error from an error stack
void *pop (Stack *stack) {
	if (stack->len <= 0){return NULL;} // double check.

	// resize stack
	if(resizestack(stack)){return NULL;}

	stack->len--; // decrement len
	if (stack->len < 0){return NULL;}

	return stack->stack[stack->len]; // pass reference to Error
}

// pushes error onto an error stack
int push (Stack *stack, void *v) {

	// resize stack
	if(resizestack(stack)){return 1;}

	stack->len++; // increment len
	if (stack->len <= 0){return 1;}

	if (v == NULL) {return 1;}

	// Push to created space on stack
	stack->stack[stack->len - 1] = v;
	return 0;
}

// "dumps" all values
int resetstack(Stack *stack) {
	stack->len = 0; // zero errors in stack
	return 0;
}

#endif // STACK