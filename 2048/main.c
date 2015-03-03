#include "../tinyGUI/tinyGUI.h"
#include "MainWindow.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR nCmdLine, int nCmdShow){
	srand(GetTickCount());
	displayWindow((Window)newMainWindow(hInstance), nCmdShow);
	return 0;
}