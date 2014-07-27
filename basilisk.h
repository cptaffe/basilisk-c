#import "util/concurrent.h" // MutexStack

// Header file for things that are useful for 
// communicating between the basiliks.

// Include guard
#ifndef BASILISK
#define BASILISK

// Basilisk struct, for random info storing
typedef struct {
	char *name;
	FILE *stream;
	MutexStack *tok; // token stack
	int cond; // condition to wait on
} Basilisk;

#endif // BASILISK