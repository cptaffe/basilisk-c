#import <strings.h>
#import "../util/gerr.h" // errors
#import "../util/concurrent.h" // MutexStack

// eof
const int itemEOF = -1;

// error
const int itemErr = 0;

// list indicator
const int itemBeginList = 5;
const int itemEndList = 6;

// whitespace (\n, \t, & ' ')
const int itemSeparator = 10;

// atoms
const int itemOp = 20;
const int itemNum = 21;
const int itemChar = 22;
const int itemStr = 23;

// Token
typedef struct {
	int type; // type number
	int line; // line number
	int ch; // char on line number
	char *str; // lexed text
} Token;

// create a lasting error, given an error where elements
// will go out of scope.
Token *_copytok (Token *tok) {
	// Create new error
	Token *token = malloc(sizeof (Token)); // allocate on heap.
	if (token == NULL){free(token); return NULL;} // token check.

	// copy ints from value
	token->type = tok->type;
	token->line = tok->line;
	token->ch = tok->ch;
	// Allocate space, pointer may go out of scope.
	// We are using this for strlcpy so there is no overflow.
	size_t size = 100 * sizeof (char);

	token->str = (char *) malloc(size);
	if (token->str == NULL){return NULL;} // token
	strlcpy(token->str, tok->str, (long) size);

	return token;
}

// pops error from an error stack
Token *poptok (MutexStack *stack) {
	return mpop(stack);
}

// pushes error onto an error stack
int pushtok (MutexStack *stack, Token *tok) {
	// move token to heap memory
	Token *token = _copytok(tok);
	if (token == NULL) {return 1;}
	return mpush(stack, token);
}

// flusherr pops all the errors and returns an array.
int resettok (Stack *stack) {
	return resetstack(stack);
}