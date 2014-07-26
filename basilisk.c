#include "basilisk-lex.h" // lexer
#include "basilisk-parse.h" // parser
#include "thread.h" // concurrency
#include "basilisk.h" // Basilisk type

// Basilisk main, launches both parser and lexer.

int main(int argc, char *argv[]) {
	Basilisk b;
	if (argc > 1){
		b.name = argv[1];
		b.stream = fopen(argv[1], "r");
		if (b.stream == NULL){return 1;}
	} else {return 1;}
	b.tok = initmstack();
	Stack *stack = initstack();

	// spawn lexer & parser
	pspawn(stack, lex, (void *) &b); 
	pspawn(stack, parse, (void *) &b);

	// wait for lexer & parser
	for (int i = 0; i < 2; i++){pwait(stack);}

	freemstack(b.tok);
	freestack(stack);
}