// miscilaneous tokens
const int itemErr = 0;
const int itemBeginList = 1;
const int itemEndList = 2;

const int itemSpace = 10;
const int itemNewLine = 11;

// operation tokens
const int itemAdd = 20;
const int itemSub = 21;
const int itemMul = 22;
const int itemDiv = 23;

// data tokens
const int itemNum = 30;

int isOp(int op) {
	switch (op) {
	case '+':
		return itemAdd;
	case '-':
		return itemSub;
	case '*':
		return itemMul;
	case '/':
		return itemDiv;
	default:
		return 0;
	}
}

int isOpCode(int op) {
	return op <= itemDiv && op >= itemAdd;
}