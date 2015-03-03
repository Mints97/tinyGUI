#include "../tinyGUI/tinyGUI.h"
#include "MainWindow.h"

/* Make the constructors */
#define FIELD(type, name, val) INIT_FIELD(type, name, val)
#define DEF_FIELD(type, name) DEFINE_FIELD(type, name, )
#define VIRTUAL_METHOD(classType, type, name, args) INIT_VIRTUAL_METHOD(classType, type, name, args)

PRIVATE void swapTiles(Tile tiles[][4], int i, int j, int i1, int j1){
	Tile temp = tiles[i][j];
	tiles[i][j] = tiles[i1][j1];
	tiles[i1][j1] = temp;
}

PRIVATE struct coords findFarthest(MAKE_THIS(MainWindow), Tile tile, int xDirection, int yDirection){
	int nextX = tile->gridXPos, nextY = tile->gridYPos, prevX, prevY;
	struct coords nextPos;

	do {
		prevX = nextX; prevY = nextY;
		nextX = prevX + xDirection; nextY = prevY + yDirection;
	} while (nextX < 4 && nextY < 4 && nextX >= 0 && nextY >= 0 && 
		(!this->tiles[nextY][nextX] || (this->tiles[nextY][nextX] && this->tiles[nextY][nextX]->number == tile->number 
			&& !this->tiles[nextY][nextX]->merged)));

	nextPos.x = prevX; nextPos.y = prevY;
	return nextPos;
}

PRIVATE void addRandomTile(MAKE_THIS(MainWindow)){ SELFREF_INIT;
	int x, y, value = rand() % 10 < 9 ? 2 : 4; /* Same condition as it original 2048 */
	while (x = rand() % 4, y = rand() % 4, this->tiles[y][x]);
	this->tiles[y][x] = newTile(this->panel->moduleInstance, value, x, y);

	$(this->panel)_addChild((GUIObject)(this->tiles[y][x]));
	displayControl((Control)(this->tiles[y][x]));

	SendMessageA(this->tiles[y][x]->handle, WM_PAINT, (WPARAM)NULL, (LPARAM)NULL); /* Force repaint after the object has been created */
}

PRIVATE void slideTiles(MAKE_THIS(MainWindow), int xDirection, int yDirection){ SELFREF_INIT;
	int traversX[4], traversY[4], i, y, j, x;
	struct coords farthestCoords;
	BOOL moved = FALSE;

	for (i = 0; i < 4; i++){
		traversX[i] = xDirection < 0 ? i : 3 - i;
		traversY[i] = yDirection < 0 ? i : 3 - i;
	}

	for (i = 0, y = traversY[i]; i < 4; i++, y = traversY[i]){
		for (j = 0, x = traversX[j]; j < 4; j++, x = traversX[j]){
			if (this->tiles[y][x]){
				farthestCoords = findFarthest(this, this->tiles[y][x], xDirection, yDirection);

				if (farthestCoords.x != x || farthestCoords.y != y){
					if (this->tiles[farthestCoords.y][farthestCoords.x]){ /* We need a merger! */
						deleteTile(this->tiles[farthestCoords.y][farthestCoords.x]);
						this->tiles[farthestCoords.y][farthestCoords.x] = NULL;
						this->tiles[y][x]->merged = TRUE;
						$(this->tiles[y][x])_incNumber();
					}

					$(this->tiles[y][x])_moveTile(farthestCoords.y, farthestCoords.x);
					swapTiles(this->tiles, y, x, farthestCoords.y, farthestCoords.x);
					moved = TRUE;
				}
			}
		}
	}

	if (moved){
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				if (this->tiles[i][j])
					this->tiles[i][j]->merged = FALSE;
		addRandomTile(this);
	}

	flushMessageQueue();
}

PRIVATE void onKeyDown(GUIObject sender, void *context, EventArgs e){
	MainWindow window = (MainWindow)sender;

	switch (e->wParam){
		case VK_LEFT:
			slideTiles(window, -1, 0);
			break;
		case VK_RIGHT:
			slideTiles(window, 1, 0);
			break;
		case VK_UP:
			slideTiles(window, 0, -1);
			break;
		case VK_DOWN:
			slideTiles(window, 0, 1);
			break;
	}
}

/* The constructor */
MainWindow newMainWindow(HINSTANCE hInstance){ SELFREF_INIT;
	ALLOC_THIS(MainWindow);
	int i, j;
	
	this->moduleInstance = hInstance;
	CLASS_MainWindow;
	initWindow((Window)this, hInstance, (char*)windowTitle, windowWidth, windowHeight);

	$(this)_addChild((GUIObject)(this->panel));

	//$(this->panel)_addChild((GUIObject)this->btn);


	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++)
			(this->tiles)[i][j] = NULL;
	}

	addRandomTile(this);
	addRandomTile(this);



	this->panel->styles = WS_CHILD | WS_BORDER | WS_THICKFRAME;
	$(this->panel)_setResizable(FALSE);
	$(this)_setEvent(WM_KEYDOWN, onKeyDown, NULL, SYNC);

	this->panel->maxWidth = this->panel->width;
	this->panel->maxHeight = this->panel->height;

	this->panel->x = 10;
	this->panel->y = 10;

	return this;
}

void deleteMainWindow(MAKE_THIS(MainWindow)){
	this->className = NULL;
	freeGUIObjectFields((GUIObject)this);
	free(this);
}

#undef FIELD
#undef DEF_FIELD
#undef VIRTUAL_METHOD