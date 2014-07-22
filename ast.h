// Abstract Syntax Tree

typedef struct {
	int type;
	int line;
	int ch;
} Node;

typedef struct {
	Node *tree; // variable length array
	int len; // length of variable length array
} List;

typedef struct {
	Node *node; // node for this
	List *tree; // tree
} Tree;

// print tree
int tprint(Tree *t) {
	return 0;
}