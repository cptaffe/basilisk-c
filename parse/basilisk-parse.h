#import <stdlib.h> // calloc, exit, etc.
#import "../tok/tok.h" // token header
#import "ast.h" // abstract syntax tree
#import "parse.h" // generic parse header
#import "../util/state.h" // state machine
#import "../basilisk.h" // Basilisk type

// Parser for some lisp-like langauge

// Parsers
// parserss absorb text using next & backup
// they record errors when needed.

const int parsenAll = 0;
const int parsenList = 1;
const int parsenOp = 2;

Token *pnext(Parser *p) {
	Token *t = mnext(p->tok);
	if (t->type == itemEOF){return NULL;}
	else if (t->type == itemErr){perr(p, t, t->str, 0); p->errors++;}
	return t;
}

// parse for beginning and end to list
int parseAll(void *v) {
	Parser *p = (Parser *) v;
	Token *t;
	while((t = pnext(p)) != NULL){
		if (t->type == itemBeginList){
			p->parenDepth++; return parsenList;
		} else if (t->type == itemEndList){
			p->parenDepth--;
			if (p->parenDepth > 0) {return 1;}
			else if (p->parenDepth < 0) {
				perr(p, t, "too many parens", 0);
				p->parenDepth++; // avoid further errors
			}
			return parsenAll;
		}
	}
	return -1;
}

// parse for op in list
int parseList(void *v) {
	Parser *p = (Parser *) v;
	Token *t;
	while((t = pnext(p)) != NULL){
		if (t->type == itemOp) {
			return parsenOp; // parseOp
		}
	}
	return -1;
}

// parse aguments of op
int parseOp(void *v) {
	Parser *p = (Parser *) v;
	Token *t;
	while((t = pnext(p)) != NULL){
		if (t->type == itemNum || t->type == itemChar || t->type == itemStr) {
		} else if (t->type == itemEndList || t->type == itemBeginList) {
			mbackup(p->tok); return parsenAll;
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

	// create root of ast
	p.root = malloc(sizeof(AsTree));
	p.root->node = NULL; // no node on root
	const int len = 3;
	p.root->tree = calloc(len, sizeof(Node *)); // allocate node pointers
	p.root->len = len;
	p.tree = p.root; // equate root to tree.

	// Set up parse func array
	stateFun parsers[] = {parseAll, parseList, parseOp};

	/*Token *tok;
	while ((tok = (Token *) mnext(p.tok)) != NULL) {
		if (tok->type == itemEOF){gnote("EOF"); break;}
		if (tok->type == itemErr){perr(&p, tok, tok->str, 0);}
		else {
			char *str = calloc(30, sizeof (char));
			sprintf(str, "%d: %s", tok->type, tok->str);
			gnote(str);
		}
	}
	//return NULL;*/

	state(parsers, &p);

	if (p.errors > 0 || p.warns > 0) {
		char str[30];
		sprintf(str, "%d errors, %d warning.", p.errors, p.warns);
		gnote(str); // general note
	}
	return NULL;
}

