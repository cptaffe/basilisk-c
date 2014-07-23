#include <stdio.h>
#include <stdlib.h> // exit
#include <strings.h> // strerror, strcpy & strlen
#include <errno.h> // errno

// General Errors
// General Errors are shared error functions.

// Error
// stores parameters for _err call.
typedef struct {
	int line; // line #
	int ch; // characters of line
	int c; // color # - for err type text
	int b; // bold # (0 off, 1 on) - for message
	int diag; // diagnostic (0 off, 1 on)
	char *str; // error text
	char *err; // error type text
	char *name; // name of file errors came from
	FILE *stream;
	// diagnostic fields
	char *read;
	int *rdlen;
} Error;

// _err
// is for compile errors (not general)

const int maxErr = 10; // 10 is a good amount

// Gerr, Gnote, & Gperr
// are for general errors encountered.

// general terminal errors
int gterr (char *str) {
	char strg[100];
	int i = sprintf(strg, "\033[1m\033[30m%s: \033[31mfatal error:\033[0m %s\n", "basilisk", str);
	fwrite(strg, i, sizeof(char), stderr);
	exit(1);
}

// general errors
int gerr (char *str) {
	char strg[100];
	int i = sprintf(strg, "\033[1m\033[30m%s: \033[31merror:\033[0m %s\n", "basilisk", str);
	return fwrite(strg, i, sizeof(char), stderr);
}

// general perror
int gperr () {
	char str[50];
	int i = sprintf(str, "\033[1m\033[30m%s: \033[31merror:\033[0m %s", "basilisk", strerror(errno));
	return fwrite(str, i, sizeof(char), stderr);
}

// general note
int gnote (char *str) {
	char strg[100];
	int i = sprintf(strg, "\033[1m\033[30m%s:\033[0m %s\n", "basilisk", str);
	return fwrite(strg, i, sizeof(char), stderr);
}

// diagnostic
int _diag (Error *err) {
	char str[100]; // 100 characters
	char *arrow = "\033[1m\033[32m^\033[0m\n";
	int i;
	// subtracts one to account for \n character.
	for (i = 0; i < (*err->rdlen - 1); i++) {
		str[i] = err->read[i];
	}; str[i++] = '\n';
	for (int j = 0; j < (err->ch - 1); j++) {
		str[i++] = ' ';
	}; strcpy(&str[i++], arrow);
	i += (strlen(arrow) / sizeof(char));
	fwrite(str, i, sizeof(char), err->stream);
	return 0;
}

// configurable error
int _err (Error *err) {
	char strg[100];
	int i = sprintf(strg, "\033[1m%s:%d:%d \033[%dm%s:\033[0m\033[%dm %s\033[0m\n", err->name, err->line, err->ch, err->c, err->err, err->b, err->str);
	i = fwrite(strg, i, sizeof(char), err->stream);
	if (err->diag){_diag(err);} // optional diagnostic
	return i;
}

// Error Stack
// queing of errors,
// pusherr is used to push errors onto a stack
// poperr is used to pop errors from the stack

const int buf = 2; // try not to realloc too much.

// Error stack
typedef struct {
	Error *err; // stack of error pointers.
	int len; // length of stack
	int max;
} ErrorStack;

// pops error from an error stack
Error *poperr (ErrorStack *stack) {
	if (stack->len <= 0){return NULL;} // double check.
	stack->len--;
	// make the error pointer free-standing
	Error *err;
	err = (stack->err + stack->len); // pass reference to Error

	// resize stack
	if ((stack->max - stack->len) > buf) {
		stack->err = realloc(stack->err, stack->len * sizeof(Error*));
		if (stack->err == NULL){return NULL;}
	}
	return err; // still on heap memory, must be freed
}

// pushes error onto an error stack
int pusherr (ErrorStack *stack, Error *err) {
	// allocate space on the stack
	stack->len++;

	// automatic stack expansion.
	if (stack->len > stack->max){
		stack->err = realloc(stack->err, ((stack->len + buf) * sizeof(Error*)));

		if (stack->err == NULL){
			return 0;
		}
		stack->max = stack->len + buf; // new max is len
	}

	// Create new error, deviod of "may go out of scope" things.
	Error *error = malloc(sizeof(Error)); // allocate on heap.
	// copy ints from value
	error->line = err->line;
	error->ch = err->ch;
	error->c = err->c;
	error->b = err->b;
	error->diag = err->diag;
	error->stream = err->stream; // copy pointer
	if (error->diag) {
		// copy pointers
		error->read = err->read;
		error->rdlen = err->rdlen;
	}
	// Allocate space, pointer may go out of scope.
	error->str = calloc(100, sizeof(char)); // message limit 100 char
	error->str = strcpy(error->str, err->str);
	error->err = calloc(20, sizeof(char)); // type limit 20 char
	error->err = strcpy(error->err, err->err);
	error->name = calloc(20, sizeof(char)); // name limit 20 char
	error->name = strcpy(error->name, err->name);

	// Push to created space on stack
	// err is an array of *Error, error is an *Error
	*(stack->err + (stack->len - 1)) = *error; // err must be heap memory.
	return 1;
}