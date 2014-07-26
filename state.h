// Unified state machine structures and functions

// Include guard
#ifndef STATE
#define STATE

// state function signature
typedef int (*stateFun) (void *v);

// State Machine
// the state machine takes an array of type stateFun functions,
// which it calls until one returns -1, which kills the function.

// State machine
int state (stateFun state[], void *v) {
	
	for (int f = 0; f != -1;) {
		f = state[f](v);
	}

	return 0;
}

#endif // STATE