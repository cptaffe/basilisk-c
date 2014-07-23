#include <stdio.h> // printing
#include <stdlib.h> // calloc, exit, etc.
#include "tok.h" // token header
#include "gerr.h" // general errors
#include "ast.h" // abstract syntax tree

// Parser for some lisp-like langauge

// Token type
typedef struct {
	int type;
	char *str;
	int line;
	int ch;
} Token;

// Parser type
typedef struct {
	FILE *stream; // stream of file
	char *name; // name of file
	char *prog; // program name
	Token *stok; // stored tok (backup)
	int errors;
	int warns;
	int parenDepth;
	Tree *root; // root of tree
	Tree *tree; // current location in tree
} Parser;

// Errors
// note, warn, err, terr, in order of least to most fatal.
// err and terr are of equal magnitude, but terr exits in case
// one cannot return -1 to the state machine.
// each function makes a call to _err, which is contained in gerr.h.

// perr (parse error), a simplified wrapper for _err
void perr(Parser *p, Token *t, char *str, int c, int b, char *err, int diag) {
	flockfile(stderr);
	Error ptr = { .read = t->str, .rdlen = &t->ch, .line = t->line, .ch = t->ch, .c = c, .b = b, .diag = diag, .str = str, .err = err, .name = p->name, .stream = stderr };
	_err(&ptr);
	// if (diag){_diag(p, t);}
	funlockfile(stderr); // release lock
}

// general errors
void err (Parser *p, Token *t, char *str, int diag) {
	p->errors++; // increment error count
	perr(p, t, str, 31, 1, "error", diag);
}

// terminal errors
void terr (Parser *p, Token *t, char *str, int diag) {
	err(p, t, str, diag); 
	exit(1); // terminates
}

// warnings
void warn (Parser *p, Token *t, char *str, int diag) {
	p->warns++; // increment error count
	perr(p, t, str, 35, 1, "warning", diag);
}

// note
void note (Parser *p, Token *t, char *str, int diag) {
	perr(p, t, str, 30, 0, "note", diag);
}

// read text until a delim is found from stream
int read(FILE *stream, char delim, char *str, int len) {
	int i;
	for (i = 0; i < len; i++) {
		char c = getc(stream);
		if (c == EOF){return EOF;}
		else if (c == ':') {str[i] = '\0'; return 0;}
		else {str[i] = c;} // assign to char[]
	}
	str[i] = '\0'; return 0;
}

// Next & Backup
// next gets the next token from the file stream in p.
// backup puts the last token back into the file stream.

const char delim = ':'; // set delim to ':'

// next token
Token *next (Parser *p) {
	// check for stored token
	if (p->stok != NULL) { 
		Token *t = p->stok;
		p->stok = NULL;
		return t;
	}
	const int len = 100;
	char *c = calloc(len, sizeof(char)); int ret;
	Token *t = malloc(sizeof(Token));
	// read type from stream
	ret = read(p->stream, delim, c, len);
	if (ret == EOF){return NULL;} else{t->type = atoi(c);}
	// read line num from stream
	ret = read(p->stream, delim, c, len);
	if (ret == EOF){return NULL;} else{t->line = atoi(c);}
	// read char num from stream
	ret = read(p->stream, delim, c, len);
	if (ret == EOF){return NULL;} else{t->ch = atoi(c);}
	// read string from stream
	ret = read(p->stream, delim, c, len);
	if (ret == EOF){return NULL;} else{t->str = c;}
	return t; // return token
}

int backup(Parser *p, Token *t) {
	p->stok = t; // set stored token to a token
	return 0;
}

// Parsers
// parserss absorb text using next & backup
// they record errors when needed.

// parse function signature
typedef int (*parse) (Parser *p);

// parse for beginning and end to list
int parseAll(Parser *p) {
	Token *t;
	while((t = next(p)) != NULL){
		if (t->type == itemBeginList){
			p->parenDepth++; return 1; // parseList
		} else if (t->type == itemEndList){
			p->parenDepth--;
			if (p->parenDepth > 0) {return 1;}
			else if (p->parenDepth < 0) {
				err(p, t, "too many parens", 0);
				p->parenDepth++; // avoid further errors
			}
			return 0;
		} // parseAll
	}
	return -1;
}

// parse for op in list
int parseList(Parser *p) {
	Token *t;
	while((t = next(p)) != NULL){
		if (isOpCode(t->type)) {
			puts(t->str); putc(':', stdout);
			return 2; // parseOp
		}
	}
	return -1;
}

// parse aguments of op
int parseOp(Parser *p) {
	Token *t;
	while((t = next(p)) != NULL){
		if (t->type == itemNum) {
			puts(t->str); putc(',', stdout);
		} else if (t->type == itemEndList) {
			backup(p, t); return 0;
		}
	}
	return -1;
}

// Parser init
// parsers is an array of all parsers.
// main creates a parser struct and acts as a state machine,
// calling the state returned by the last state function
// until that state is -1, then exiting.

// Set up parse func array
parse parsers[3] = {parseAll, parseList, parseOp};

int main (int argc, char *argv[]) {
	// Init lexer
	Parser p;
	p.prog = argv[0]; // set prog to program name

	// check for filename
	if (argc > 1) {
		p.name = argv[1];
		p.stream = fopen(p.name, "r"); // open file in read mode
		if (p.stream == NULL) {
			gperr(); return 1;
		}
	} else {
		// default to stdin
		p.name = "stdin";
		p.stream = stdin;
	}

	// create root of ast
	p.root = malloc(sizeof(Tree));
	p.root->node = NULL; // no node on root
	p.root->tree = malloc(sizeof(List)); // tree
	const int len = 3;
	p.root->tree->tree = calloc(len, sizeof(Node)); // realloc for more nodes
	p.root->tree->len = len;
	p.tree = p.root; // equate root to tree.

	// state machine
	for (int f = 0; f != -1;) {
		f = parsers[f](&p);
	}

	fclose(p.stream); // close input

	if (p.errors > 0 || p.warns > 0) {
		char str[30];
		sprintf(str, "%d errors, %d warning.", p.errors, p.warns);
		gnote(str); // general note
	}
}

