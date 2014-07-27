#import <stdio.h> // stdio for printing.
#import "../util/gerr.h" // general errors
#import "../tok/tok.h" // tokens
#import "../util/concurrent.h" // MutexStack

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
	Stack *err; // error buffer
	MutexStack *tok; // token stack

} Lexer;

// Errors
// These functions all lerr (lex error) whith their error text,
// the error is then added to l.err, an ErrorStack, using pusherr,
// flusherr MUST be called at nemit before the current line is
// overwritten, otherwise the error will fail.

// lerr (lex error)
// Stores all parameters to a buffer.
void llerr (Lexer *l, char *str, int c, int b, char *err, int diag, int past) {
	// create error (references may go out of scope).
	Error ptr = { .read = l->str, .rdlen = &l->e, .line = l->lineNum, .ch = l->e, .c = c, .b = b, .diag = diag, .str = str, .err = err, .name = l->name, .past = past};

	// push error to error stack.
	int e = pusherr(l->err, &ptr);
	if(e){gterr("pusherr didn't push.");} // error check

	// Flush errors if above maximum que.
	//if (l->err->len > maxQue) {flusherr(l);}
}

int tokerr(MutexStack *stack, Error *err) {
	Token tok = {.type = itemErr, .line = err->line, .ch = err->ch, .str = err->str};
	return pushtok(stack, &tok);
}

// flusherr (Flush Errors)
void flusherr (Stack *stack, MutexStack *tokens) {
	// call _err on all Errors
	int len = stack->len; // save length, is reduced in poperr
	if (len == 0){return;}

	// Instead of popping errors, loop through error stack,
	// and then reset the stack to a length of 0.
	for (int i = 0; i < len; i++) {
		Error *err = stack->stack[i];
		tokerr(tokens, err); // signal error
		// free allocated errors
		free(err->str); free(err->err); free(err->name);
		free(err); // free actual error
	}
	reseterr(stack); // set errors to 0
}

// Diagnostic errors
// errors for diagnostic functions in errors such as pointing out
// offending characters or numbers of characters

// diagnostic lexical errors
// can highlight past characters
void lderr (Lexer *l, char *str, int diag, int past) {
	l->errors++; // increment error count
	llerr(l, str, 31, 1, "error", diag, past);
}

// terminal lexical errors
void ldterr (Lexer *l, char *str, int diag, int past) {
	llerr(l, str, 31, 1, "fatal error", diag, past); 
	exit(1); // terminates
}

// lexical warnings
void ldwarn (Lexer *l, char *str, int diag, int past) {
	l->warns++; // increment error count
	llerr(l, str, 35, 1, "warning", diag, past);
}

// lexical notes
void ldnote (Lexer *l, char *str, int diag, int past) {
	llerr(l, str, 30, 0, "note", diag, past);
}

// diag err Wrappers for different styles of errors.
// note, warn, err, terr, in order of least to most fatal.
// err and terr are of equal magnitude, but terr exits in case
// one cannot return -1 to the state machine.

// general lexical errors
void lerr (Lexer *l, char *str) {lderr(l, str, 0, 0);}

// terminal lexical errors
void lterr (Lexer *l, char *str) {ldterr(l, str, 0, 0);}

// lexical warnings
void lwarn (Lexer *l, char *str) {ldwarn(l, str, 0, 0);}

// lexical notes
void lnote (Lexer *l, char *str) {ldnote(l, str, 0, 0);}

// Emit
// emit and nemit record tokens and print them.
// nemit also records a newline, which is needed for accurate
// error printing.

// emit to token stack
int l_emit (Lexer *l, int n) {
	// create Token
	char *str = malloc(100 * sizeof (char));
	strlcpy(str, &l->str[l->b], (l->e - l->b) + 1);
	Token tok = {.type = n, .line = l->lineNum, .ch = l->b, .str = str};
	l->b = l->e;
	return pushtok(l->tok, &tok);
}

// lreset resets the lexer to the zeroth index
void lreset (Lexer *l) {
	// errors depend on text the scanner keeps.
	flusherr(l->err, l->tok); // flushes all errors
	l->e = 0; l->b = 0; // resets storage of characters
}

// Next & Backup
// next gets the next character from the file stream in l.
// backup puts the last character back into the file stream
// and goes back one character in the char array.

// next character
char lnext (Lexer *l) {
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
int lbackup (Lexer *l) {
	if ((l->e - l->b) > 0){
		l->e--;
		ungetc(l->str[l->e], l->stream);
		return 0;
	} else {lterr(l, "negative index");} // error
	return 1; // should never reach
}