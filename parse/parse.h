#import <stdio.h> // printing
#import <stdlib.h> // calloc, exit, etc.
#import "../util/gerr.h" // general errors

// Parser type
typedef struct {
	char *name; // name of file
	int errors;
	int warns;
	int parenDepth;
	MutexStack *tok; // tok
	int len; // tok is read from bottom, length read
	AsTree *root; // root of tree
	AsTree *tree; // current location in tree
} Parser;

// Errors
// note, warn, err, terr, in order of least to most fatal.
// err and terr are of equal magnitude, but terr exits in case
// one cannot return -1 to the state machine.
// each function makes a call to _err, which is contained in gerr.h.

// perr (parse error), a simplified wrapper for _err
void pperr(Parser *p, Token *t, char *str, int c, int b, char *err, int diag) {
	Error ptr = { .read = t->str, .rdlen = &t->ch, .line = t->line, .ch = t->ch, .c = c, .b = b, .diag = diag, .str = str, .err = err, .name = p->name };
	_err(&ptr, stderr);
}

// general errors
void perr (Parser *p, Token *t, char *str, int diag) {
	p->errors++; // increment error count
	pperr(p, t, str, 31, 1, "error", diag);
}

// terminal errors
void pterr (Parser *p, Token *t, char *str, int diag) {
	perr(p, t, str, diag); 
	exit(1); // terminates
}

// warnings
void pwarn (Parser *p, Token *t, char *str, int diag) {
	p->warns++; // increment error count
	pperr(p, t, str, 35, 1, "warning", diag);
}

// note
void pnote (Parser *p, Token *t, char *str, int diag) {
	pperr(p, t, str, 30, 0, "note", diag);
}
