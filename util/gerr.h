#import <stdio.h>
#import <stdlib.h> // exit
#import <strings.h> // strerror, strcpy & strlen
#import <errno.h> // errno
#import "stack.h"
#import "concurrent.h" // MutexStack

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
	int past; // underline past characters
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
	// + 1 is important because of null byte at end of string.
	size_t size = sizeof pref + ((strlen(str) + 1) * sizeof (char));
	char *msg = malloc(size);
	strlcpy(msg, pref, size);
	size_t len = strlcat(msg, str, size); // add prefix
	len = strlcat(msg, "\n", len + (2 * sizeof (char))); // add newline
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

int counttabs (char *s, int len) {
	int tabs = 0;
	for (int i = 0; i < len; i++){
		if (s[i] == '\t'){tabs++;}
	}
	return tabs;
}

// diagnostic
int _diag (Error *err, FILE *stream) {
	char str[200]; // 100 characters
	char arrow[] = "\033[1m\033[32m^\033[0m\n";
	char undln[] = "\033[1m\033[32m~";
	int i;
	// copy content to local string
	i = strlcpy(str, err->read, *err->rdlen + 1);
	// append newline, only if not already appended
	if (err->read[*err->rdlen - 1] != '\n'){
		i = strlcat(str, "\n", (*err->rdlen + 2) * sizeof (char));
	}
	int tabs = counttabs(err->read, *err->rdlen); // tab count
	int j;
	if (err->past > 0){err->past--;} // only the number of undls
	// fills tabs
	for (j = 0; j < tabs; j++) {
		str[i] = '\t'; i+= sizeof (char);}
	// fills spaces
	for (; j < (err->ch - (err->past + 1)); j++) {
		str[i] = ' '; i += sizeof (char);}
	// fills undls
	for (; j < (err->ch - 1); j++){
		strlcpy(&str[i], undln, sizeof undln);
		i += sizeof undln / sizeof (char);}
	strlcpy(&str[i], arrow, sizeof arrow);
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

// create a lasting error, given an error where elements
// will go out of scope.
Error *_copyerr (Error *err) {
	// Create new error
	Error *error = malloc(sizeof (Error)); // allocate on heap.
	if (error == NULL){free(error); return NULL;} // error check.

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
		error->past = err->past;
	}
	// Allocate space, pointer may go out of scope.
	// We are using this for strlcpy so there is no overflow.
	size_t size = 100 * sizeof (char);

	error->str = (char *) malloc(size);
	if (error->str == NULL){return NULL;} // error
	strlcpy(error->str, err->str, (long) size);

	error->err = (char *) malloc(size);
	if (error->err == NULL){return NULL;} // error
	strlcpy(error->err, err->err, (long) size);

	error->name = (char *) malloc(size);
	if (error->name == NULL){return NULL;} // error
	strlcpy(error->name, err->name, (long) size);

	return error;
}

// pops error from an error stack
Error *poperr (Stack *stack) {
	return (Error *) pop(stack);
}

// pushes error onto an error stack
int pusherr (Stack *stack, Error *err) {
	Error *error = _copyerr(err);
	if (error == NULL) {return 1;}
	return push(stack, (void *) error);
}

int reseterr(Stack *stack) {
	return resetstack(stack);
}

// finerr (Final Error)
// prints final error, either emitting a gnote for the number of
// errors and warnings or that no errors were emitted.
void finerr (int errors, int warns) {
	if (errors > 0 || warns > 0) {
		char str[30];
		int i;
		if (errors == 1){i = sprintf(str, "%d error, ", errors);}
		else{i = sprintf(str, "%d errors, ", errors);}
		if (warns == 1){sprintf(&str[i-1], " %d warning.", warns);}
		else{sprintf(&str[i-1], " %d warnings.", warns);}
		gnote(str); // general note
	} else {gnote("no errors emitted.");}
}