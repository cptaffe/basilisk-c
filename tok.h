
// Token
typedef struct {
	char *tok;
	int type;
} Token;

// miscilaneous tokens
const int itemSpace = 0;
const int itemBeginList = 1;
const int itemEndList = 2;

// operation tokens
const int itemAdd = 10;
const int itemSub = 11;

// data tokens
const int itemNum = 20;