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
	const char pref[] = "\033[1m\033[30mbasilisk:\033[0m ";
	// + 2 is important because of null byte at end of string.
	size_t size = sizeof pref + ((strlen(str) + 5) * sizeof (char));
	char *msg = malloc(size);
	strlcpy(msg, pref, size);
	size_t len = strlcat(msg, str, size); // add prefix
	len = strlcat(msg, "\n", len + 5); // add newline
	fwrite(msg, len, sizeof (char), stderr);
	fflush(stderr);
}

// function used to do a simple sprintf and call to _gerr.
void _gsprerr (const char *str, const char *msg) {
	char strg[100]; // fuck you man, constant size ftw!
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
	const char msg[] = "\033[1m\033[31merror:\033[0m %s";
	char str[sizeof msg + 50];
	sprintf(str, msg, strerror(errno));
	_gerr(str);
}

// general warning
// Use case: a helpful not of some type.
// General notes
void gwarn (const char *str) {
	const char *msg = "\033[1m\033[35mwarning:\033[0m %s";
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
	char arrow[] = "\033[1m\033[32m^\033[0m\n";
	int i;
	// subtracts one to account for \n character.
	for (i = 0; i < (*err->rdlen - 1); i++) {
		str[i] = err->read[i];
	}; str[i++] = '\n';
	for (int j = 0; j < (err->ch - 1); j++) {
		str[i++] = ' ';
	}; strcpy(&str[i++], arrow);
	i += sizeof arrow / sizeof (char);
	fwrite(str, i, sizeof (char), stream);
	return 0;
}

// configurable error
int _err (Error *err, FILE *stream) {
	char strg[100];
	int i = sprintf(strg, "\033[1m%s:%d:%d \033[%dm%s:\033[0m\033[%dm %s\033[0m\n", err->name, err->line, err->ch, err->c, err->err, err->b, err->str);
	i = fwrite(strg, i, sizeof (char), stream);
	if (err->diag){_diag(err, stream);} // optional diagnostic
	return i;
}

// Error Stack
// queing of errors,
// pusherr is used to push errors onto a stack
// poperr is used to pop errors from the stack
// NOTE: strings such as those used for _diag are stored
// by reference, and thusly, these errors MUST BE FLUSHED
// before the pointer is freed or loses the information
// it was meant to carry.
// BAD NOTE: this bit is admitedly fucked up, but it should
// work for legitimate amounts of errors (in my experience, it has)

const int ErrorStackBuf = 5; // try not to realloc too much.

// Error stack
typedef struct {
	Error *err; // stack of error pointers.
	int len; // length of stack
	int max;
} ErrorStack;

int resizerr (ErrorStack *stack) {
	int grow, shrink;
	grow = stack == NULL || stack->len >= stack->max;
	//shrink = stack->max - stack->len > ErrorStackBuf;
	shrink = 0;
	if (!grow && !shrink) {return 0;}
	
	// reallocate len+buf # of Error pointer positions
	int buff;
	if (grow){buff = ErrorStackBuf;}
	else{buff = 0;}
	size_t size = (stack->len + buff) * sizeof (Error *);
	stack->err = (Error *) realloc(stack->err, size);
	if (stack->err == NULL){free(stack->err); return 1;}
	stack->max = size / sizeof (Error *); // new max is len
	return 0;
}

// pops error from an error stack
Error *poperr (ErrorStack *stack) {
	if (stack->len <= 0){return NULL;} // double check.

	// resize stack
	if(resizerr(stack)){return NULL;}

	stack->len--;
	// make the error pointer free-standing
	Error *err;
	err = (stack->err + stack->len); // pass reference to Error

	return err; // still on heap memory, must be freed
}

// pushes error onto an error stack
int pusherr (ErrorStack *stack, Error *err) {

	// resize stack
	int resize = resizerr(stack);
	if(resize){return resize;}

	stack->len++; // increment len
	if (stack->len <= 0){return 1;}

	// Create new error
	Error *error = (Error *) malloc(sizeof (Error)); // allocate on heap.
	if (error == NULL){free(error); return 1;} // error check.

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
	size_t size = 100 * sizeof (char);

	error->str = (char *) malloc(size);
	if (error->str == NULL){return 1;} // error
	strlcpy(error->str, err->str, (long) size);

	error->err = (char *) malloc(size);
	if (error->err == NULL){return 1;} // error
	strlcpy(error->err, err->err, (long) size);

	// name is a trusted pointer
	error->name = err->name;
	/*error->name = (char *) malloc(size);
	if (error->name == NULL){return 1;} // error
	strlcpy(error->name, err->name, (long) size);*/

	// Push to created space on stack
	// err is an array of *Error, error is an *Error
	*(stack->err + (stack->len - 1)) = *error; // err must be heap memory.
	return 0;
}