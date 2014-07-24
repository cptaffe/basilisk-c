//#include <stdio.h>
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
	// diagnostic fields
	char *read;
	int *rdlen;
} Error;

// Gerr, Gnote, & Gperr
// General Errors, these errors are to be used for general,
// non-lexical errors.

// function used to print all general errors.
void _gerr (const char *str) {
	const char *pref = "\033[1m\033[30mbasilisk:\033[0m ";
	// + 2 is important because of null byte at end of string.
	int size = ((strlen(pref) + strlen(str)) * sizeof(char) + 2);
	char *msg = malloc(size);
	strlcpy(msg, pref, size);
	int len = strlcat(msg, str, size); // add prefix
	len = strlcat(msg, "\n", len + 5); // add newline
	fwrite(msg, len, sizeof(char), stderr);
	fflush(stderr);
}

// function used to do a simple sprintf and call to _gerr.
void _gsprerr (const char *str, const char *msg) {
	char strg[strlen(msg) + strlen(str) + 2];
	sprintf(strg, msg, str);
	_gerr(strg);
}

// general terminal errors
void gterr (const char *str) {
	const char *msg = "\033[1m\033[31mfatal error:\033[0m %s";
	_gsprerr(str, msg);
	exit(1);
}

// general errors
void gerr (const char *str) {
	const char *msg = "\033[1m\033[31merror:\033[0m %s";
	_gsprerr(str, msg);
}

// general perror
// Use case: realloc returned a NULL pointer, gperr will print why.
// Used like the perror call, it prints the string associated with
// the current errno value.
void gperr () {
	const char *msg = "\033[1m\033[31merror:\033[0m %s";
	char str[strlen(msg) + 50];
	sprintf(str, msg, strerror(errno));
	_gerr(str);
}

// general warning
// Use case: a helpful not of some type.
// General notes
void gwarn (const char *str) {
	const char *msg = "\033[1m\033[31mwarn:\033[0m %s";
	_gsprerr(str, msg);
}

// general note
// Use case: a helpful not of some type.
// General notes
void gnote (const char *str) {
	_gerr(str);
}

// diagnostic
int _diag (Error *err, FILE *stream) {
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
	fwrite(str, i, sizeof(char), stream);
	return 0;
}

// configurable error
int _err (Error *err, FILE *stream) {
	char strg[100];
	int i = sprintf(strg, "\033[1m%s:%d:%d \033[%dm%s:\033[0m\033[%dm %s\033[0m\n", err->name, err->line, err->ch, err->c, err->err, err->b, err->str);
	i = fwrite(strg, i, sizeof(char), stream);
	if (err->diag){_diag(err, stream);} // optional diagnostic
	return i;
}

// Error Stack
// queing of errors,
// pusherr is used to push errors onto a stack
// poperr is used to pop errors from the stack

const int ErrorStackBuf = 2; // try not to realloc too much.

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
	if ((stack->max - stack->len) > ErrorStackBuf) {
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
		// reallocate len+buf # of Error pointer positions
		stack->err = realloc(stack->err, ((stack->len + ErrorStackBuf) * sizeof(Error*)));

		// if realloc failed, return 0.
		if (stack->err == NULL){
			return 0;
		}
		stack->max = stack->len + ErrorStackBuf; // new max is len
	}

	// Create new error, deviod of "may go out of scope" things.
	Error *error = malloc(sizeof(Error)); // allocate on heap.
	// copy ints from value
	error->line = err->line;
	error->ch = err->ch;
	error->c = err->c;
	error->b = err->b;
	error->diag = err->diag;
	if (error->diag) {
		// copy pointers
		error->read = err->read;
		error->rdlen = err->rdlen;
	}
	// Allocate space, pointer may go out of scope.
	error->str = calloc(strlen(err->str), sizeof(char));
	error->str = strcpy(error->str, err->str);
	error->err = calloc(strlen(err->err), sizeof(char));
	error->err = strcpy(error->err, err->err);
	error->name = calloc(strlen(err->name), sizeof(char));
	error->name = strcpy(error->name, err->name);

	// Push to created space on stack
	// err is an array of *Error, error is an *Error
	*(stack->err + (stack->len - 1)) = *error; // err must be heap memory.
	return 1;
}