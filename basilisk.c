#import "lex/basilisk-lex.h" // lexer
#import "parse/basilisk-parse.h" // parser
#import "util/thread.h" // concurrency
#import "util/gerr.h" // general errors
#import "basilisk.h" // Basilisk type

// Basilisk main, launches both parser and lexer.

int main(int argc, char *argv[]) {
	Basilisk b;
	if (argc > 1){
		b.name = argv[1];
		b.stream = fopen(argv[1], "r");
		if (b.stream == NULL){return 1;}
	} else {
		gnote("reading from stdin");
		b.name = "stdin";
		b.stream = stdin;
	}
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