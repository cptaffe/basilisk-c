// Include guard.
#ifndef AST
#define AST

// Abstract Syntax Tree

typedef struct {
	int type;
	int line;
	int ch;
} Node;

Node *_copynode (Node *node) {
	if (node == NULL){return NULL;}

	// Create new error
	Node *nd = malloc(sizeof (Node)); // allocate on heap.
	if (nd == NULL){free(nd); return NULL;} // token check.

	// copy ints from value
	nd->type = node->type;
	nd->line = node->line;
	nd->ch = node->ch;

	return nd;
}

typedef struct {
	Node *node;
	void **tree; // AsTree stack (void to avoid recursive definition)
	int len;
	int max;
} AsTree;

const int AstStackBuf = 5;

// create a lasting error, given an error where elements
// will go out of scope.
AsTree *_copyast (AsTree *ast) {
	if (ast == NULL){return NULL;}

	// Create new error
	AsTree *tree = malloc(sizeof (AsTree)); // allocate on heap.
	if (tree == NULL){free(tree); return NULL;} // token check.

	tree->node = _copynode(ast->node); // copy node
	for (int i = 0; i < ast->len; i++){
		tree->tree[i] = _copyast(ast->tree[i]); // recursive copy
	}
	tree->len = ast->len;

	return tree;
}

int resizeast (AsTree *tree) {
	int grow, shrink;
	grow = tree == NULL || tree->len >= tree->max;
	shrink = tree->max - tree->len > AstStackBuf;
	if (!grow && !shrink) {return 0;}
	
	// reallocate len+buf # of Error pointer positions
	int buff;
	if (grow){buff = AstStackBuf;}
	else{buff = 0;}
	size_t size = (tree->len + buff) * sizeof (AsTree *);
	tree->tree = (void **) realloc(tree->tree, size);
	if (tree->tree == NULL){free(tree->tree); return 1;}
	tree->max = size / sizeof (AsTree *); // new max is len
	return 0;
}

// pops error from an error stack
Token *popast (AsTree *tree) {
	if (tree->len <= 0){return NULL;} // double check.

	// resize stack
	if(resizeast(tree)){return NULL;}

	tree->len--; // decrement len
	if (tree->len < 0){return NULL;}

	return tree->tree[tree->len]; // pass reference to Error
}

// pushes error onto an error stack
int pushast (AsTree *tree, AsTree *tr) {

	// resize stack
	if(resizeast(tree)){return 1;}

	tree->len++; // increment len
	if (tree->len <= 0){return 1;}

	// move error to heap memory
	AsTree *trc = _copyast(tr);
	if (trc == NULL) {return 1;}

	// Push to created space on stack
	// err is an array of *Error, error is an *Error
	tree->tree[tree->len - 1] = trc; // err must be heap memory.
	return 0;
}

// flusherr pops all the errors and returns an array.
int resetast (AsTree *tree) {
	tree->len = 0; // zero errors in stack
	return 0;
}

#endif // AST