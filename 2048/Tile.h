MAKE_TYPEDEF(Tile);



#define BOARD_SIZE 400



#define CLASS_Tile /* : */ CLASS_Label \
	FIELD(int, number, 0); \
	FIELD(int, gridXPos, 0); \
	FIELD(int, gridYPos, 0); \
	FIELD(BOOL, merged, FALSE); \
	FIELD(Brush, bgBrush, newBrush(BS_SOLID, RGB(0xEE, 0xE4, 0xDA), (ULONG_PTR)NULL));

	METHOD(Tile, void, moveTile, (MAKE_THIS(Tile), int row, int col));
	METHOD(Tile, void, incNumber, (MAKE_THIS(Tile)));

	#define _moveTile(row, col) MAKE_METHOD_ALIAS(Tile, moveTile(CURR_THIS(Tile), row, col))
	#define _incNumber() MAKE_METHOD_ALIAS(Tile, incNumber(CURR_THIS(Tile)))



/* =============================================================================================== */



/* Make the class */
#define FIELD(type, name, val) MAKE_FIELD(type, name, val)
#define DEF_FIELD(type, name) MAKE_FIELD(type, name, )
#define VIRTUAL_METHOD(classType, type, name, args) MAKE_VIRTUAL_METHOD(classType, type, name, args)

MAKE_CLASS(Tile);

#undef FIELD
#undef DEF_FIELD
#undef VIRTUAL_METHOD


/* =============================================================================================== */

/* The constructor and destructor */
Tile newTile(HINSTANCE hInstance, int number, int gridXPos, int gridYPos);
MAKE_DEFAULT_DESTRUCTOR_PROTOTYPE(Tile);
