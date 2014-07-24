#include <stdio.h> // printf, putc
#include <stdlib.h> // calloc, exit
#include <signal.h> // signal handler
#include "tok.h" // token header
#include "lex.h" // lexical scanning library.

// Lexer for some lisp-like langauge

// Lexers
// lexers absorb text using next & backup
// they emit text using emit and nemit (\n)
// they record errors when needed.

// lex function signature
typedef int (*lex) (Lexer *l);

int emit(Lexer *l, int n) {
	if (n == itemNewLine){lreset(l);}
	return _emit(l, n);
}

// looks for begenning and end of lists
int lexAll (Lexer *l) {
	char c;
	while((c = next(l)) != EOF) {
		if (c == '\n'){emit(l, itemNewLine);} // mark newline
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
		if (c == '\n'){emit(l, itemNewLine);} // mark newline
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
		if (c == '\n'){emit(l, itemNewLine);} // mark newline
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
	if (c == '\n'){emit(l, itemNewLine);} // mark newline
	return -1; // die at EOF
}

// Lexer init
// lexers is an array of all lexers
// main creates a lexer struct and acts as a state machine,
// calling the state returned by the last state function
// until that state is -1, then exiting.

// Set up lex func array
lex lexers[3] = {lexAll, lexList, lexOp};

void sighandle (int signal) {
	if (signal == 2){gerr("fuck you man."); return;}
	gterr(strsignal(signal));
}

int main (int argc, char *argv[]) {

	// Init lexer on stack memory
	Lexer l = {
		.length = 100, // set length to 100
		.lineNum = 1, // first line
		.errstream = stderr
	};

	//l.outstream = stdout;
	l.outstream = fopen("out.txt", "w");
	if (l.outstream == NULL){gperr(); return 1;}

	// allocate ErrorStack on heap memory
	l.err = (ErrorStack *) malloc(sizeof (ErrorStack));
	if (l.err == NULL){gperr(); return 1;}

	l.err->err = NULL;
	l.err->len = 0;
	l.err->max = 0;


	// allocate l.length characters of space
	l.str = calloc(l.length, sizeof (char)); // zeroed memory
	if (l.str == NULL) {gperr(); return 1;}

	// check for filename
	if (argc > 1) {
		l.name = argv[1];
		l.stream = fopen(l.name, "r"); // open file in read mode
		if (l.stream == NULL) {gperr(); return 1;}
	} else {
		// default to stdin
		l.name = "stdin";
		l.stream = stdin;
	}

	// signal handler, because wtf not? ami right?
	/*for (int i = 1; i <= 31; i++){
		signal(i, sighandle);
	}*/ // handle all the signals!

	// DEBUG TESTING
	/*for (int i = 0; i < 100; i++) {
		for (int j = 0; j < i; j++) {
			char c[20];
			sprintf(c, "fuck this %d %d", i, j);
			err(&l, c, 0);
		}
		flusherr(&l);
	}
	return 0;*/

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