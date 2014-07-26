#include <stdlib.h> // calloc, exit, etc.
#include "tok.h" // token header
#include "ast.h" // abstract syntax tree
#include "parse.h" // generic parse header
#include "state.h" // state machine
#include "basilisk.h" // Basilisk type

// Parser for some lisp-like langauge

// Parsers
// parserss absorb text using next & backup
// they record errors when needed.

// parse for beginning and end to list
int parseAll(void *v) {
	Parser *p = (Parser *) v;
	Token *t;
	while((t = mnext(p->tok)) != NULL){
		if (t->type == itemBeginList){
			p->parenDepth++; return 1; // parseList
		} else if (t->type == itemEndList){
			p->parenDepth--;
			if (p->parenDepth > 0) {return 1;}
			else if (p->parenDepth < 0) {
				perr(p, t, "too many parens", 0);
				p->parenDepth++; // avoid further errors
			}
			return 0;
		} // parseAll
	}
	return -1;
}

// parse for op in list
int parseList(void *v) {
	Parser *p = (Parser *) v;
	Token *t;
	while((t = mnext(p->tok)) != NULL){
		if (isOpCode(t->type)) {
			puts(t->str); putc(':', stdout);
			return 2; // parseOp
		}
	}
	return -1;
}

// parse aguments of op
int parseOp(void *v) {
	Parser *p = (Parser *) v;
	Token *t;
	while((t = mnext(p->tok)) != NULL){
		if (t->type == itemNum) {
			puts(t->str); putc(',', stdout);
		} else if (t->type == itemEndList) {
			mbackup(p->tok); return 0;
		}
	}
	return -1;
}

// Parser init
// parsers is an array of all parsers.
// main creates a parser struct and acts as a state machine,
// calling the state returned by the last state function
// until that state is -1, then exiting.

void *parse (void *v) {
	Basilisk *b = (Basilisk *) v;

	// Init lexer
	Parser p = {.errors = 0, .warns = 0};

	p.name = b->name;
	p.tok = b->tok;

	p.err = initstack();
	if (p.err == NULL){gerr("stack didn't init"); return NULL;}

	// create root of ast
	p.root = malloc(sizeof(AsTree));
	p.root->node = NULL; // no node on root
	const int len = 3;
	p.root->tree = calloc(len, sizeof(Node *)); // allocate node pointers
	p.root->len = len;
	p.tree = p.root; // equate root to tree.

	// Set up parse func array
	stateFun parsers[] = {parseAll, parseList, parseOp};

	Token *tok;
	while ((tok = (Token *) mnext(p.tok)) != NULL) {
		if (tok->type == itemEOF){break;}
		char *str = calloc(30, sizeof (char));
		sprintf(str, "recieved: %s", tok->str);
		gnote(str);
	}
	return NULL;

	state(parsers, &p);

	if (p.errors > 0 || p.warns > 0) {
		char str[30];
		sprintf(str, "%d errors, %d warning.", p.errors, p.warns);
		gnote(str); // general note
	}
	return NULL;
}

