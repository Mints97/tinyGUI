#include "tile.h"


/* Coordinates on the board */
struct coords {
	int x, y;
};


MAKE_TYPEDEF(MainWindow);





#define CLASS_MainWindow /* : */ CLASS_Window \
	DEF_FIELD(int, nCmdShow); \
	FIELD(Window, panel, newWindow(this->moduleInstance, NULL, BOARD_SIZE, BOARD_SIZE)); \
	FIELD(Button, btn, newButton(this->moduleInstance, "test button", 10, 10, 100, 20)); \
	DEF_FIELD(Tile, tiles[4][4]);

	static const char *windowTitle = "Memory Game";
	static const int windowWidth = 700;
	static const int windowHeight = 500;



/* =============================================================================================== */



/* Make the class */
#define FIELD(type, name, val) MAKE_FIELD(type, name, val)
#define DEF_FIELD(type, name) MAKE_FIELD(type, name, )
#define VIRTUAL_METHOD(classType, type, name, args) MAKE_VIRTUAL_METHOD(classType, type, name, args)

MAKE_CLASS(MainWindow);

#undef FIELD
#undef DEF_FIELD
#undef VIRTUAL_METHOD


/* =============================================================================================== */

/* The constructor and destructor */
MainWindow newMainWindow(HINSTANCE hInstance);
MAKE_DEFAULT_DESTRUCTOR_PROTOTYPE(MainWindow);
