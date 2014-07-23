#include <stdio.h> // printf, putc
#include <stdlib.h> // calloc, exit
#include <strings.h> // strcpy
#include "tok.h" // token header
#include "gerr.h" // general errors

// Lexer for some lisp-like langauge

// Lexer struct
typedef struct {
	FILE *stream; // stream of file
	FILE *outstream; // stream out
	char *name; // name of file
	char *str; // string read
	int e; // end of string
	int b; // begenning of string
	int length;
	int lineNum;
	int parenDepth;
	int errors;
	int warns;
	ErrorStack *err; // error buffer
} Lexer;

// Errors
// note, warn, err, terr, in order of least to most fatal.
// err and terr are of equal magnitude, but terr exits in case
// one cannot return -1 to the state machine.

// print stack length DEBUG!
void dbprint (Lexer *l, const char *pref) {
	char *str = malloc(30 * sizeof(char));
	int i = sprintf(str, "%s: ", pref);
	i += sprintf((str + i), "stack len: %d", l->err->len);
	sprintf((str + i), " max: %d.", l->err->max);
	gerr(str);
	free(str);
}

// qerr (Que Errors)
// called on nemit (newline emit) so that all of l.str is avaliable
// no strcpy or pointer update is needed, because the string is stored
// in the Lexer struct.
void flusherr (Lexer *l) {
	// call _err on all Errors
	//dbprint(l, "flush");
	int len = l->err->len;
	if (len == 0){gerr("no errors to emit."); return;}
	else {char c[30]; sprintf(c, "len: %d", len); gnote(c);}
	flockfile(stderr); // aquire lock
	Error *que[len];
	for (int i = 0; i < len; i++) {
		Error *err = poperr(l->err); // pop error
		if (err == NULL){gterr("poperr didn't pop."); return;}
		err->stream = stderr;
		puts("TEST...");
		_err(err);
		que[i] = err;
	}
	for (int i = len; i > 0; i--) {
		Error *err = que[i-1];
		_err(err); // signal error
		// free allocated errors
		free(err->str); // free str char []
		free(err->err); // free err char []
		free(err->name); // free err char []
	}
	funlockfile(stderr); // release lock
}

// prints final error.
void finerr (Lexer *l) {
	if (l->errors > 0 || l->warns > 0) {
		char str[30];
		sprintf(str, "%d errors, %d warning.", l->errors, l->warns);
		gnote(str); // general note
	} else {gnote("no errors emitted.");}
}

// lerr (lex error)
// Stores all parameters to a buffer that is emptied when
// qerr is called.
void lerr (Lexer *l, char *str, int c, int b, char *err, int diag) {
	//dbprint(l, "lex err");
	// qerr must be called BEFORE some references expire.
	// create error (references may go out of scope).
	Error ptr = { .read = l->str, .rdlen = &l->e, .line = l->lineNum, .ch = l->e, .c = c, .b = b, .diag = diag, .str = str, .err = err, .name = l->name };

	// push error to error stack.
	int e = pusherr(l->err, &ptr);
	if(!e){gterr("pusherr didn't push.");} // error check

	if ((l->errors + l->warns) > maxErr) {
		flusherr(l); // flush last errors
		finerr(l); // final error
		gterr("too many errors");
	}
}

// general errors
void err (Lexer *l, char *str, int diag) {
	l->errors++; // increment error count
	lerr(l, str, 31, 1, "error", diag);
}

// terminal errors
void terr (Lexer *l, char *str, int diag) {
	lerr(l, str, 31, 1, "fatal error", diag); 
	exit(1); // terminates
}

// warnings
void warn (Lexer *l, char *str, int diag) {
	l->warns++; // increment error count
	lerr(l, str, 35, 1, "warning", diag);
}

// note
void note (Lexer *l, char *str, int diag) {
	lerr(l, str, 30, 0, "note", diag);
}

// Emit
// emit and nemit record tokens and print them.
// nemit also records a newline, which is needed for accurate
// error printing.

// emit
int emit (Lexer *l, int n) {
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

// emits and records new line
// This must be used when emitting a newline,
// otherwise the newline will not be recorded as
// an incrementation of the line number.
int nemit (Lexer *l, int n) {
	flusherr(l);
	int e = emit(l, n);
	l->lineNum++; l->e = 0; l->b = 0;
	return e;
}

// Next & Backup
// next gets the next character from the file stream in l.
// backup puts the last character back into the file stream
// and goes back one character in the char array.

// next character
char next (Lexer *l) {
	if (l->e < l->length){
		char c;
		if ((c = getc(l->stream)) != EOF){
			l->str[l->e] = c;
		} else {return EOF;} // deferr error handling.
		l->e++;
		return c; // returns char
	} else {
		err(l, "line too long", 0);
		emit(l, itemErr); // dump line
	}
	return 1; // should never reach
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

// Lexers
// lexers absorb text using next & backup
// they emit text using emit and nemit (\n)
// they record errors when needed.

// lex function signature
typedef int (*lex) (Lexer *l);

// looks for begenning and end of lists
int lexAll (Lexer *l) {
	char c;
	while((c = next(l)) != EOF) {
		if (c == '\n'){nemit(l, itemNewLine);} // mark newline
		else if (c == '(') {
			emit(l, itemBeginList);
			l->parenDepth++;
			return 1;
		} else if (c == ')') {
			l->parenDepth--;
			if (l->parenDepth < 0){
				err(l, "too many parens", 1);
				l->parenDepth++;
			}
			emit(l, itemEndList);
			if (l->parenDepth > 0) {
				return 1;
			} else{return 0;}
		}
	}
	return -1; // EOF
}

// looks for op tokens
int lexList (Lexer *l) {
	char c = next(l);
	if (c == EOF || c == '\n'){
		if (c == '\n'){nemit(l, itemNewLine);} // mark newline
		err(l, "list prematurely ends", 1); return 0;
	}
	int op;
	if ((op = isOp(c))) {
		emit(l, op);
		return 2; // lexOp
	}else {err(l, "not an operation", 1);}
	return 1; // should never reach here.
}

// lexes n number of integers
int lexOp (Lexer *l) {
	char c;
	c = next(l); 
	if (c == EOF || c == '\n'){
		if (c == '\n'){nemit(l, itemNewLine);} // mark newline
		err(l, "operation ends without args", 1); return 0;
	} else if (c == ' '){
		emit(l, itemSpace);
	} else {
		warn(l, "missing space after op", 1); 
		backup(l);
	} // get rid of space
	while ((c = next(l)) != EOF && c != '\n') {
		// eat numbers, not a number...
		if (c > '9' || c < '0') {
			backup(l); // get off not a number
			if ((l->e - l->b) > 0) {
				emit(l, itemNum);
				if ((c = next(l)) != ' ' && c != ')'){
					err(l, "nonnumeric in op", 1);
				} else if (c == ' ') {
					emit(l, itemSpace);
				} else{backup(l); return 0;} // lexAll
			} else if ((c = next(l)) == ' '){
				warn(l, "extra space", 1);
			} else {
				err(l, "unknown character", 1);
			}
		}
	}
	if (c == '\n'){nemit(l, itemNewLine);} // mark newline
	return -1; // die at EOF
}

// Lexer init
// lexers is an array of all lexers
// main creates a lexer struct and acts as a state machine,
// calling the state returned by the last state function
// until that state is -1, then exiting.

// Set up lex func array
lex lexers[3] = {lexAll, lexList, lexOp};

int main (int argc, char *argv[]) {

	// Init lexer
	Lexer l = {
		.length = 100, // set length to 100
		.lineNum = 1, // first line
	};

	//l.outstream = stdout;
	l.outstream = fopen("out.txt", "w");
	if (l.outstream == NULL){gperr();}

	// NULL to signal to realloc that this is not alloc'd yet.
	l.err = calloc(1, sizeof(ErrorStack));


	// allocate l.length characters of space
	l.str = calloc(l.length, sizeof(char)); // zeroed memory
	if (l.str == NULL) {
		gperr(); return 1;
	}

	// check for filename
	if (argc > 1) {
		l.name = argv[1];
		l.stream = fopen(l.name, "r"); // open file in read mode
		if (l.stream == NULL) {
			gperr(); return 1;
		}
	} else {
		// default to stdin
		l.name = "stdin";
		l.stream = stdin;
	}

	// DEBUG TESTING
	for (int i = 0; i < 11; i++) {
		for (int j = 0; j < i; j++) {
			char c[20];
			sprintf(c, "fuck this %d %d", i, j);
			err(&l, c, 0);
		}
		flusherr(&l);
	}

	return 0;

	// state machine
	// This is where all the magic happens
	for (int f = 0; f != -1;) {
		f = lexers[f](&l);
	}

	// Clean up by free-ing (important)
	free(l.str);

	flusherr(&l); // clear last errors, if any.
	fclose(l.stream); // close input
	fclose(l.outstream); // close input

	finerr(&l); // final err
}