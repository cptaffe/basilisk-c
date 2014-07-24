#include <stdio.h> // stdio for printing.
#include "gerr.h" // general errors

// Copyright (c) 2014 by Connor Taffe, licensed under
// the MIT license.

// lex.h is purposed for lexical scanning of programs.
// It provides machinery for erroring, emitting tokens,
// keeping accurate line and character counts, etc.
// It is assumed that the functions for some state machine will
// be provided to the state machine, allowing lex.h to be used for
// any programming language.
// The purpose of lex.h is to allow for any language to be simply
// lexed by using these functions.

// Lexer struct
typedef struct {
	FILE *stream; // stream of file
	FILE *outstream; // stream out
	FILE *errstream; // stream to error
	char *name; // name of file
	char *str; // string read
	int e; // end of string
	int b; // begenning of string
	int length; // length of str
	int lineNum; // line number
	int parenDepth; // depth of parenthesis
	int errors; // number of errors
	int warns; // number of warns
	ErrorStack *err; // error buffer
} Lexer;

const int maxErr = 10;

// Errors
// These functions all lerr (lex error) whith their error text,
// the error is then added to l.err, an ErrorStack, using pusherr,
// flusherr MUST be called at nemit before the current line is
// overwritten, otherwise the error will fail.

// finerr (Final Error)
// prints final error, either emitting a gnote for the number of
// errors and warnings or that no errors were emitted.
void finerr (Lexer *l) {
	if (l->errors > 0 || l->warns > 0) {
		char str[30];
		int i;
		if (l->errors == 1){i = sprintf(str, "%d error, ", l->errors);}
		else{i = sprintf(str, "%d errors, ", l->errors);}
		if (l->warns == 1){sprintf(&str[i-1], " %d warning.", l->warns);}
		else{sprintf(&str[i-1], " %d warnings.", l->warns);}
		gnote(str); // general note
	} else {gnote("no errors emitted.");}
}

// flusherr (Flush Errors)
// called on nemit (newline emit) so that all of l.str is avaliable
// no strcpy or pointer update is needed, because the string is stored
// in the Lexer struct.
void flusherr (Lexer *l) {
	// call _err on all Errors
	int len = l->err->len; // save length, is reduced in poperr
	if (len == 0){return;}
	// Initial checks complete
	flockfile(l->errstream); // aquire lock
	// Add poped errors to que
	Error *que[len];
	for (int i = 0; i < len; i++) {
		Error *err = poperr(l->err); // pop error
		if (err == NULL){gterr("poperr didn't pop."); return;}
		que[i] = err;
	}
	// error poped errors in opposite order.
	for (int i = len; i > 0; i--) {
		Error *err = que[i-1];
		_err(err, l->errstream); // signal error
		// free allocated errors
		free(err->str); // free str char []
		free(err->err); // free err char []
		//free(err->name); // free err char []
		//free(err);
	}
	fflush(l->errstream); // just in case
	funlockfile(l->errstream); // release lock

	if (l->errors >= maxErr) {
		finerr(l); // final error
		gterr("too many errors");
	}
}

// lerr (lex error)
// Stores all parameters to a buffer.
void lerr (Lexer *l, char *str, int c, int b, char *err, int diag) {
	// create error (references may go out of scope).
	Error ptr = { .read = l->str, .rdlen = &l->e, .line = l->lineNum, .ch = l->e, .c = c, .b = b, .diag = diag, .str = str, .err = err, .name = l->name };

	// push error to error stack.
	int e = pusherr(l->err, &ptr);
	if(e){gterr("pusherr didn't push.");} // error check

	// Flush errors if above maximum que.
	//if (l->err->len > maxQue) {flusherr(l);}
}

// lerr Wrappers for different styles of errors.
// note, warn, err, terr, in order of least to most fatal.
// err and terr are of equal magnitude, but terr exits in case
// one cannot return -1 to the state machine.

// general lexical errors
void err (Lexer *l, char *str, int diag) {
	l->errors++; // increment error count
	lerr(l, str, 31, 1, "error", diag);
}

// terminal lexical errors
void terr (Lexer *l, char *str, int diag) {
	lerr(l, str, 31, 1, "fatal error", diag); 
	exit(1); // terminates
}

// lexical warnings
void warn (Lexer *l, char *str, int diag) {
	l->warns++; // increment error count
	lerr(l, str, 35, 1, "warning", diag);
}

// lexical notes
void note (Lexer *l, char *str, int diag) {
	lerr(l, str, 30, 0, "note", diag);
}

// Emit
// emit and nemit record tokens and print them.
// nemit also records a newline, which is needed for accurate
// error printing.

// emit
int _emit (Lexer *l, int n) {
	const char delim = ':'; // set delim to ':'
	fprintf(l->outstream, "%d", n); putc(delim, l->outstream);
	fprintf(l->outstream, "%d", l->lineNum); putc(delim, l->outstream);
	fprintf(l->outstream, "%d", l->e); putc(delim, l->outstream);
	for (int i = l->b; i < l->e; i++) {
		putc(l->str[i], l->outstream);
	}
	putc(delim, l->outstream);
	l->b = l->e; // shifts begin to end pos
	fflush(l->outstream); // flushes buffer to stdout
	return 0;
}

// lreset resets the lexer to the zeroth index
void lreset (Lexer *l) {
	// errors depend on text the scanner keeps.
	flusherr(l); // flushes all errors
	l->e = 0; l->b = 0; // resets storage of characters
}

// Next & Backup
// next gets the next character from the file stream in l.
// backup puts the last character back into the file stream
// and goes back one character in the char array.

// next character
char next (Lexer *l) {
	if (l->e >= l->length){
		l->str = realloc(l->str, (l->e + 10) * sizeof (char));
	}
	char c;
	if ((c = getc(l->stream)) != EOF){
		l->str[l->e] = c;
		if (c == '\n'){l->lineNum++;} // update line num
	} else {return EOF;} // deferr error handling.
	l->e++;
	return c; // returns char
}

// backup one character
int backup (Lexer *l) {
	if ((l->e - l->b) > 0){
		l->e--;
		ungetc(l->str[l->e], l->stream);
		return 0;
	} else {terr(l, "negative index", 0);} // error
	return 1; // should never reach
}