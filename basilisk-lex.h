#include <stdio.h> // printf, putc
#include <stdlib.h> // calloc, exit
#include <signal.h> // signal handler
#include "tok.h" // token header
#include "lex.h" // lexical scanning library.
#include "state.h" // state machine
#include "basilisk.h" // Basilisk type

// Lexer for some lisp-like langauge

// Lexers
// lexers absorb text using next & backup
// they emit text using emit and nemit (\n)
// they record errors when needed.

void lemit(Lexer *l, int n) {
	l_emit(l, n);
	if (n == itemNewLine){lreset(l);}
}

// is s a space character?
int isspace (char s) {return s == ' ' || s == '\t';}

// are there characters not emitted?
int unemitted (Lexer *l) {return l->e > l->b;}

// function index constants
const int lexnAll = 0;
const int lexnList = 1;
const int lexnOp = 2;
const int lexnSpace = 3;
const int lexnNum = 4;

// looks for begenning and end of lists
int lexAll (void *v) {
	Lexer *l = (Lexer *) v;
	char c;
	while((c = lnext(l)) != EOF) {
		if (c == '\n'){lemit(l, itemNewLine);} // mark newline
		else if (c == '(') {
			lemit(l, itemBeginList);
			l->parenDepth++;
			return lexnList;
		} else if (c == ')') {
			l->parenDepth--;
			if (l->parenDepth < 0){
				lderr(l, "too many parens", 1, 0);
				l->parenDepth++;
			}
			lemit(l, itemEndList);
			if (l->parenDepth > 0) {
				return lexnList;
			} else{return lexnAll;}
		} else {lemit(l, itemErr);} // dump junk
	}
	return -1; // EOF
}

// looks for op tokens
int lexList (void *v) {
	Lexer *l = (Lexer *) v;
	char c = lnext(l);
	if (c == EOF){lderr(l, "list prematurely ends", 1, 0); return -1;}
	else if (c == '\n'){lemit(l, itemNewLine);} // mark newline
	else if (isspace(c)){lbackup(l); return lexnSpace;}
	else if (isOp(c)) {lbackup(l); return lexnOp;}
	else if (c < '9' && c > '0'){lbackup(l); return lexnNum;}
	else if (c == ')' || c == '('){lbackup(l); return lexnAll;}
	else {lemit(l, itemErr);} // dump junk
	return lexnList; // should never reach here.
}

// lexes some number of integers
int lexOp (void *v) {
	Lexer *l = (Lexer *) v;
	char c = lnext(l); 
	int op = isOp(c);
	if (op){lemit(l, op);}
	else{
		if(unemitted(l)){lemit(l, itemErr);} // dump junk
		else{lerr(l, "lexOp did not find any ops");}
	}
	return lexnList;
}

// lexes some number of spaces
// accepts tabs and spaces
int lexSpace (void *v) {
	Lexer *l = (Lexer *) v;
	char c;
	while((c = lnext(l)) != EOF) {
		// absorb spaces
		if (!isspace(c)){
			lbackup(l);
			if (unemitted(l)){lemit(l, itemSpace);}
			else{lerr(l, "lexSpace did not find any spaces");}
			return lexnList;
		}
	}
	lerr(l, "unexpected EOF");
	return -1;
}

// lexes numbers
int lexNum (void *v) {
	Lexer *l = (Lexer *) v;
	char c;
	while((c = lnext(l)) != EOF) {
		// absorb numbers
		if (c > '9' || c < '0'){
			lbackup(l);
			if (unemitted(l)){lemit(l, itemNum);}
			else{lerr(l, "lexNum did not find any numbers");}
			return lexnList;
		}
	}
	lerr(l, "unexpected EOF");
	return -1;
}

// Lexer init
// lexers is an array of all lexers
// main creates a lexer struct and acts as a state machine,
// calling the state returned by the last state function
// until that state is -1, then exiting.

void *lex (void *v) {
	Basilisk *b = (Basilisk *) v;

	// Init lexer on stack memory
	Lexer l = {
		.length = 100, // set length to 100
		.lineNum = 1, // first line
		.errstream = stderr
	};

	l.err = initstack();
	if (l.err == NULL){gperr(); return NULL;}

	l.tok = b->tok;

	// allocate l.length characters of space
	l.str = calloc(l.length, sizeof (char)); // zeroed memory
	if (l.str == NULL) {gperr(); return NULL;}

	l.name = b->name;
	l.stream = b->stream;

	// Set up lex func array
	stateFun lexers[] = {lexAll, lexList, lexOp, lexSpace, lexNum};

	state(lexers, &l);
	lemit(&l, itemEOF);

	flusherr(l.err, l.tok); // clear last errors, if any.

	// Free all resource, nothing can escape!
	free(l.str); // do not free Basilisk resources though.

	finerr(l.errors, l.warns); // final err
	return NULL;
}