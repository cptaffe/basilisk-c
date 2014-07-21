#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> // for errorf
#include "tok.h" // token header

// Lexer for some lisp-like langauge

// Lexer struct
typedef struct Lexer {
	FILE *stream; // stream of file
	char *name; // name of file
	char *str; // string read
	int i; // index in string
	int length;
	int lineNum;
} Lexer;

// error prints errors
int error(Lexer *l, char *str) {
	printf("\033[1m%s:%d:%d \033[31merror:\033[0m\033[1m %s\033[0m\n", l->name, l->lineNum, l->i, str);
   	exit(1);
}

char next(Lexer *l) {
	if (l->i < l->length){
		char c;
		if ((c = getc(l->stream)) != EOF){
			if (c == '\n') {
				l->lineNum++;
			}
			l->str[l->i] = c;
		} // get next char
		l->i++;
		return c; // returns char
	} else {
		error(l, "EOF");
		return 0;
	}
}

int backup(Lexer *l) {
	if (l->i > 0){
		l->i--;
		ungetc(l->str[l->i], l->stream);
		return 0;
	} else {return 1;} // error
}

int emit(Lexer *l, int n) {
	printf("%d: ", n);
	for (int i = 0; i < l->i; i++) {
		putc(l->str[i], stdout);
	}
	putc('\n', stdout);
	l->i = 0;
	return 0;
}

// lex function signature
typedef int (*lex) (Lexer *l);

int lexAll(Lexer *l) {
	char c;
	while(1) {
		c = next(l);
		if (c == '(') {
			emit(l, itemBeginList);
			return 1;
		} else if (c == ')') {
			emit(l, itemEndList);
			return 0;
		}
	}
}

int lexList(Lexer *l) {
	char c = next(l);
	switch (c) {
	case '+':
		emit(l, itemAdd);
		return 2; // lexOp
		break;
	case '-':
		emit(l, itemSub);
		return 2; // lexOp
		break;
	default:
		error(l, "not an operation");
		break;
	}
	return 1; // should never reach here.
}

int lexOp(Lexer *l) {
	char c;
	next(l); emit(l, itemSpace); // eat '+'
	while ((c = next(l)) != 0) {
		if (c == ' ' || c == '\n' || c == ')') {
			backup(l); // get off space
			if (l->i > 0) {
				emit(l, itemNum); 
				if ((c = next(l)) == ' ' || c == '\n') {emit(l, itemSpace);}
				else if (c == ')'){backup(l); return 0;}
			} else {error(l, "extra space in add");}
		}
	}
	return 1; // lexList
}

// Set up lex func array
lex lexers[3] = {lexAll, lexList, lexOp};

// general errors
int gerr(char *str) {
	printf("%s: error: %s", "basilisk", str);
	exit(1);
}

int main(int argc, char *argv[]) {
	// Init lexer
	Lexer l;
	l.str = malloc(100 * sizeof(char));
	l.i = 0;
	l.length = 100;
	l.lineNum = 0;

	// check for filename
	if (argc > 1) {
		l.name = argv[1];
		l.stream = fopen(l.name, "r"); // open file in read mode
		if (l.stream == NULL) {
			perror("file error");
		}
	} else {
		gerr("no input files found");
	}

	// state machine
	for (int f = 0; f != -1;) {
		f = lexers[f](&l);
	}
}