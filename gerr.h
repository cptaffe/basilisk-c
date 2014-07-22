#include <stdio.h>
#include <stdlib.h> // exit
#include <strings.h> // strerror
#include <errno.h> // errno

// General Errors
// General Errors are shared error functions.

// _err
// is for compile errors (not general)

const int maxErr = 10; // 10 is a good amount

// Gerr, Gnote, & Gperr
// are for general errors encountered.

// general terminal errors
int gterr (char *str) {
	char strg[100];
	int i = sprintf(strg, "\033[1m\033[30m%s: \033[31merror:\033[0m %s\n", "basilisk", str);
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

// prints configurable error
int _err (int errs, int warns, char *name, int line, int ch, char *str, int c, int b, char *err) {
	char strg[100];
	int i = sprintf(strg, "\033[1m%s:%d:%d \033[%dm%s:\033[0m\033[%dm %s\033[0m\n", name, line, ch, c, err, b, str);
	//exit if too many errors.
	i = fwrite(strg, i, sizeof(char), stderr);
	if ((errs + warns) > maxErr) {
		char str[30];
		sprintf(str, "%d errors, %d warning.", errs, warns);
		gnote(str); // general note
		gterr("too many errors");
	}
	return i;
}