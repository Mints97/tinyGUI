#include "../tinyGUI/tinyGUI.h"
#include "Tile.h"


/* Make the constructors */
#define FIELD(type, name, val) INIT_FIELD(type, name, val)
#define DEF_FIELD(type, name) DEFINE_FIELD(type, name, )
#define VIRTUAL_METHOD(classType, type, name, args) INIT_VIRTUAL_METHOD(classType, type, name, args)

METHOD(Tile, void, moveTile, (MAKE_THIS(Tile), int row, int col)){ SELFREF_INIT;
	Sleep(1);

	$(this)_setPos(col * BOARD_SIZE / 4, row * BOARD_SIZE / 4);
	this->gridXPos = col;
	this->gridYPos = row;
}

METHOD(Tile, void, incNumber, (MAKE_THIS(Tile))){ SELFREF_INIT;
	char title[7];
	this->number *= 2;
	sprintf(title, "%d", this->number);

	$(this)_setText(title);
}

void onPaint(GUIObject tile, void *context, EventArgs e){ SELFREF_INIT;
	COLORREF color;
	RECT textRect;
	HGDIOBJ prevFont;

	textRect.top = 0;
	textRect.left = 0;
	textRect.right = tile->width - 1;
	textRect.bottom = tile->height - 1;

	switch(((Tile)tile)->number){
		case 2:
		case 4:
			color = RGB(0xEE, 0xE4, 0xDA);
			break;

		case 8:
			color = RGB(0xF2, 0xB1, 0x79);
			break;

		case 16:
			color = RGB(0xF5, 0x95, 0x63);
			break;

		case 32:
			color = RGB(0xF6, 0x7C, 0x5F);
			break;

		case 64:
			color = RGB(0xF6, 0x5E, 0x3B);
			break;

		case 128:
			color = RGB(0xED, 0xCF, 0x72);
			break;

		case 256:
			color = RGB(0xED, 0xCC, 0x61);
			break;

		case 512:
			color = RGB(0xED, 0xC8, 0x50);
			break;

		case 1024:
			color = RGB(0xED, 0xC5, 0x3F);
			break;

		case 2048:
			color = RGB(0xED, 0xC2, 0x2E);
			break;

		default:
			color = RGB(0x3C, 0x3A, 0x32);
			break;
	}

	$(((Tile)tile)->bgBrush)_setBrushColor(color);

	$(tile)_drawRect(NULL, ((Tile)tile)->bgBrush, 0, 0, tile->width, tile->height);
	
	prevFont = SelectObject(tile->paintContext, GetStockObject(DEFAULT_GUI_FONT));
	SetBkMode(tile->paintContext, TRANSPARENT);
	DrawTextA(tile->paintContext, tile->text, -1, &textRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
}


/* The constructor */
Tile newTile(HINSTANCE hInstance, int number, int gridXPos, int gridYPos){ SELFREF_INIT;
	ALLOC_THIS(Tile);
	char title[7];
	if (gridXPos >= 4 || gridYPos >= 4){
		free(this);
		return NULL;
	}
	
	sprintf(title, "%d", number);

	CLASS_Tile;
	initLabel((Label)this, hInstance, title, gridXPos * BOARD_SIZE / 4, gridYPos * BOARD_SIZE / 4, BOARD_SIZE / 4, BOARD_SIZE / 4);
	this->number = number;

	this->gridXPos = gridXPos;
	this->gridYPos = gridYPos;

	this->styles |= WS_BORDER | SS_CENTER | SS_CENTERIMAGE;

	$(this)_setEvent(WM_PAINT, onPaint, NULL, SYNC);
	//$(this)_setEvent(WM_CTLCOLORSTATIC, onCtlColorStatic, NULL, SYNC);

	return this;
}

void deleteTile(MAKE_THIS(Tile)){
	this->className = NULL;
	deleteBrush(this->bgBrush);
	freeGUIObjectFields((GUIObject)this);
	free(this);
}

#undef FIELD
#undef DEF_FIELD
#undef VIRTUAL_METHOD