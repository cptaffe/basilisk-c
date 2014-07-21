#include <stdio.h> // printf, putc
#include <stdlib.h> // malloc, exit
#include "tok.h" // token header

// Lexer for some lisp-like langauge

// Lexer struct
typedef struct Lexer {
	FILE *stream; // stream of file
	char *name; // name of file
	char *str; // string read
	int e; // end of string
	int b; // begenning of string
	int length;
	int lineNum;
	int parenDepth;
} Lexer;

// Errors
// note, warn, err, terr, in order of least to most fatal.
// err and terr are of equal magnitude, but terr exits in case
// one cannot return -1 to the state machine.

// prints configurable error
int _err (Lexer *l, char *str, int c, int b, char *err) {
	return fprintf(stderr, "\033[1m%s:%d:%d \033[%dm%s:\033[0m\033[%dm %s\033[0m\n", l->name, l->lineNum, l->e, c, err, b, str);
}

int _diag (Lexer *l) {
	int b;
	if (l->e > 20){
		b = (l->e - 17); fputs("...", stderr);
	} else {b = 0;}
	for (int i = b; i < l->e; i++) {
		putc(l->str[i], stderr);
	}; putc('\n', stderr);
	for (int i = b; i < (l->e-1); i++) {
		putc(' ', stderr);
	}; fputs("\033[32m^\033[0m\n", stderr);
	return 0;
}

// general errors
void err (Lexer *l, char *str, int diag) {
	_err(l, str, 31, 1, "error");
	if (diag){_diag(l);}
}

// terminal errors
void terr (Lexer *l, char *str, int diag) {
	err(l, str, diag); 
	exit(1); // terminates
}

// warnings
void warn (Lexer *l, char *str, int diag) {
	_err(l, str, 35, 1, "warning");
	if (diag){_diag(l);}
}

// note
void note (Lexer *l, char *str, int diag) {
	_err(l, str, 30, 0, "note");
	if (diag){_diag(l);}
}

// Emit
// emit and nemit record tokens and print them.
// nemit also records a newline, which is needed for accurate
// error printing.

// emit
int emit (Lexer *l, int n) {
	printf("%d:", n);
	for (int i = l->b; i < l->e; i++) {
		putc(l->str[i], stdout);
	}
	putc('\n', stdout);
	l->b = l->e; // shifts begin to end pos
	return 0;
}

// emits and records new line
int nemit (Lexer *l, int n) {
	l->lineNum++;
	l->e = 0; l->b = 0;
	return emit(l, n);
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
	if (c == '+' || c == '-' || c == '*' || c == '/') {
		emit(l, itemOp);
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
// gerr is for general errors encountered in main.
// main creates a lexer struct and acts as a state machine,
// calling the state returned by the last state function
// until that state is -1, then exiting.

// Set up lex func array
lex lexers[3] = {lexAll, lexList, lexOp};

// general errors
int gerr (char *str) {
	printf("%s: error: %s", "basilisk", str);
	exit(1);
}

int main (int argc, char *argv[]) {
	// Init lexer
	Lexer l;
	l.str = malloc(100 * sizeof(char));
	if (l.str == NULL) {
		perror("memory allocation failure");
	}
	l.b = 0;
	l.e = 0;
	l.length = 100;
	l.lineNum = 1; // first line

	// check for filename
	if (argc > 1) {
		l.name = argv[1];
		l.stream = fopen(l.name, "r"); // open file in read mode
		if (l.stream == NULL) {
			perror("file error");
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

	note(&l, "the end is nigh", 0);
}