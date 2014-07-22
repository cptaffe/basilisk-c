#include <stdio.h> // printf, putc
#include <stdlib.h> // calloc, exit
#include <unistd.h> // sleep function DEBUG!
#include <strings.h> // strcpy
#include "tok.h" // token header
#include "gerr.h" // general errors

// Lexer for some lisp-like langauge

// Lexer struct
typedef struct {
	FILE *stream; // stream of file
	char *name; // name of file
	char *prog; // name of program
	char *str; // string read
	int e; // end of string
	int b; // begenning of string
	int length;
	int lineNum;
	int parenDepth;
	int errors;
	int warns;
} Lexer;

// Errors
// note, warn, err, terr, in order of least to most fatal.
// err and terr are of equal magnitude, but terr exits in case
// one cannot return -1 to the state machine.

int _diag (Lexer *l) {
	char str[100]; // 100 characters
	char *arrow = "\033[1m\033[32m^\033[0m\n";
	int i;
	for (i = 0; i < l->e; i++) {
		str[i] = l->str[i];
	}; str[i++] = '\n';
	for (int j = 0; j < (l->e-1); j++) {
		str[i++] = ' ';
	}; strcpy(&str[i++], arrow);
	i += (strlen(arrow) / sizeof(char));
	fwrite(str, i, sizeof(char), stderr);
	return 0;
}

// lerr (lex error), a simplified wrapper for _err
void lerr(Lexer *l, char *str, int c, int b, char *err, int diag) {
	flockfile(stderr); // aquire lock for stdout
	_err(l->errors, l->warns, l->name, l->lineNum, l->e, str, c, b, err);
	if (diag){_diag(l);}
	funlockfile(stderr); // release lock
	//sleep(2);
}

// general errors
void err (Lexer *l, char *str, int diag) {
	l->errors++; // increment error count
	lerr(l, str, 31, 1, "error", diag);
}

// terminal errors
void terr (Lexer *l, char *str, int diag) {
	err(l, str, diag); 
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
	printf("%d", n); putc(delim, stdout);
	printf("%d", l->lineNum); putc(delim, stdout);
	printf("%d", l->e); putc(delim, stdout);
	for (int i = l->b; i < l->e; i++) {
		putc(l->str[i], stdout);
	}
	putc(delim, stdout);
	l->b = l->e; // shifts begin to end pos
	fflush(stdout); // flushes buffer to stdout
	return 0;
}

// emits and records new line
int nemit (Lexer *l, int n) {
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
		if (c == '(') {
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
	Lexer l;
	l.str = calloc(100, sizeof(char)); // zeroed memory
	if (l.str == NULL) {
		gperr(); return 1;
	}
	l.length = 100;
	l.lineNum = 1; // first line
	l.prog = argv[0]; // set prog to program name

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

	// state machine
	for (int f = 0; f != -1;) {
		f = lexers[f](&l);
	}

	fclose(l.stream); // close input

	if (l.errors > 0 || l.warns > 0) {
		char str[30];
		sprintf(str, "%d errors, %d warning.", l.errors, l.warns);
		gnote(str); // general note
	}
}