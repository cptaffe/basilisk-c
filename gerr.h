#include <stdio.h>
#include <stdlib.h> // exit

// General Errors
// General Errors are shared error functions.

// _err
// is for compile errors (not general)

const int maxErr = 10; // 10 is a good amount

// Gerr, Gnote, & Gperr
// are for general errors encountered.

// general terminal errors
int gterr (char *str) {
	fprintf(stderr, "\033[1m\033[30m%s: \033[31merror:\033[0m %s\n", "basilisk", str);
	exit(1);
}

// general errors
int gerr (char *str) {
	return fprintf(stderr, "\033[1m\033[30m%s: \033[31merror:\033[0m %s\n", "basilisk", str);
}

// general perror
void gperr () {
	char str[50];
	sprintf(str, "\033[1m\033[30m%s: \033[31merror\033[0m", "basilisk");
	perror(str);
}

// general note
int gnote (char *str) {
	return fprintf(stderr, "\033[1m\033[30m%s:\033[0m %s\n", "basilisk", str);
}

// prints configurable error
int _err (int errs, int warns, char *name, int line, int ch, char *str, int c, int b, char *err) {
	int i = fprintf(stderr, "\033[1m%s:%d:%d \033[%dm%s:\033[0m\033[%dm %s\033[0m\n", name, line, ch, c, err, b, str);
	//exit if too many errors.
	if ((errs + warns) > maxErr) {
		char str[30];
		sprintf(str, "%d errors, %d warning.", errs, warns);
		gnote(str); // general note
		gterr("too many errors");
	}
	return i;
}