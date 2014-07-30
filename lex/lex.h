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
	Stack *err; // error buffer
	MutexStack *tok; // token stack

} Lexer;

// error emit
int lerr (Lexer *l, char *str) {
	Token tok = {.type = itemErr, .line = l->lineNum, .ch = l->b, .str = str};
	return pushtok(l->tok, &tok);
}

// dump characters
void ldump (Lexer *l) {
	l->b = l->e;
}

// emit to token stack
int lemit (Lexer *l, int n) {
	// create Token
	char *str = malloc(100 * sizeof (char));
	strlcpy(str, &l->str[l->b], (l->e - l->b) + 1);
	Token tok = {.type = n, .line = l->lineNum, .ch = l->b, .str = str};
	l->b = l->e;
	return pushtok(l->tok, &tok);
}

// lreset resets the lexer to the zeroth index
void lreset (Lexer *l) {
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
	}
	return 1; // should never reach
}