#import <stdio.h> // printf, putc
#import <stdlib.h> // calloc, exit
#import <signal.h> // signal handler
#import <ctype.h> // isalnum()
#import "../tok/tok.h" // token header
#import "../lex/lex.h" // lexical scanning library.
#import "../util/state.h" // state machine
#import "../basilisk.h" // Basilisk type

// Lexer for some lisp-like langauge

// Lexers
// lexers absorb text using next & backup
// they emit text using emit and nemit (\n)
// they record errors & dump text when needed.

// is s a space character?
int isseparator (char s) {return s == ' ' || s == '\t' || s == '\n';}

// are there characters not emitted?
int unemitted (Lexer *l) {return l->e > l->b;}

// function index constants
const int lexnList = 0;
const int lexnAtom = 1;
const int lexnOp = 2;
const int lexnNum = 3;
const int lexnChar = 4;
const int lexnStr = 5;

// list characters
const char beginList = '(';
const char endList = ')';

// list stuff
int lexList (void *v) {
	Lexer *l = (Lexer *) v;
	char c;
	while((c = lnext(l)) != EOF) {
		if (c == beginList) {
			l->parenDepth++;
			lemit(l, itemBeginList);
			return lexnOp;
		} else if (c == endList) {
			l->parenDepth--;
			lemit(l, itemEndList);
			if (l->parenDepth < 0) {
				lerr(l, "too many parens"); ldump(l); l->parenDepth++;
				return lexnList;}
			else if (l->parenDepth == 0){return lexnList;}
			else{return lexnList;}
		} else if (isseparator(c)){
			lemit(l, itemSeparator);
			return lexnList;
		} else {
			char str[] = "unexpected character: %c";
			sprintf(str, str, l->str[l->b]);
			lerr(l, str); ldump(l);}
	}
	return -1;
}

// lex operator
int lexOp (void *v) {
	Lexer *l = (Lexer *) v;
	char c;
	while((c = lnext(l)) != EOF) {
		// eat alphanumeric
		if (!isalnum(c)) {
			lbackup(l);
			if (unemitted(l)){lemit(l, itemOp);}
			else{lerr(l, "list missing an operator");}
			return lexnAtom;
		}
	}
	return -1;
}

// lexes atoms of any type
int lexAtom (void *v) {
	Lexer *l = (Lexer *) v;
	char c;
	while((c = lnext(l)) != EOF) {
		if (c < '9' && c > '0'){return lexnNum;}
		else if (c == '\''){return lexnChar;}
		else if (c == '\"'){return lexnStr;}
		else if (c == ')' || c == '('){lbackup(l); return lexnList;}
		else if (isseparator(c)){lemit(l, itemSeparator);}
		else{lerr(l, "unexpected chracter");}
	}
	lerr(l, "unexpected EOF");
	return -1;
}

int lexNum (void *v) {
	Lexer *l = (Lexer *) v;
	char c;
	while((c = lnext(l)) != EOF) {
		// eat number
		if (c > '9' || c < '0') {
			lbackup(l);
			if (unemitted(l)){lemit(l, itemNum);}
			else{lerr(l, "not a number");}
			return lexnAtom;
		}
	}
	return -1;
}

int lexChar (void *v) {
	Lexer *l = (Lexer *) v;
	char c;
	while((c = lnext(l)) != EOF) {
		// eat char
		if (c == '\'') {
			if (unemitted(l)){lemit(l, itemChar);}
			else{lerr(l, "not a character");}
			return lexnAtom;
		}
	}
	return -1;
}

int lexStr (void *v) {
	Lexer *l = (Lexer *) v;
	char c;
	while((c = lnext(l)) != EOF) {
		// eat string
		if (c == '\"') {
			if (unemitted(l)){lemit(l, itemStr);}
			else{lerr(l, "not a string");}
			return lexnAtom;
		}
	}
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
	stateFun lexers[] = {lexList, lexAtom, lexOp, lexNum, lexChar, lexStr};

	state(lexers, &l);

	lemit(&l, itemEOF);

	// Free all resource, nothing can escape!
	free(l.str); // do not free Basilisk resources though.
	return NULL;
}