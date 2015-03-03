#include "tinyGUI.h"

/* Get ownership of a mutex. Used in thread synchronization for accessing shared global context */
static HANDLE getMutexOwnership(char *mutexName){
	HANDLE mutex = NULL;
	DWORD mutexWaitResult;
	mutex = CreateMutexA(NULL, FALSE, mutexName); /* Create named mutex or get a handle to it if it exists */
	
	if (!mutex)
		return NULL;

	mutexWaitResult = WaitForSingleObject(mutex, 0); /* Check mutex status */

	if (mutexWaitResult == WAIT_TIMEOUT){ /* Someone owns the mutex, wait till it's released */
		mutexWaitResult = WaitForSingleObject(mutex, INFINITE);
		if (mutexWaitResult == WAIT_OBJECT_0 || mutexWaitResult == WAIT_ABANDONED)
			return mutex;
		else
			return NULL;
	} else if (mutexWaitResult == WAIT_OBJECT_0 || mutexWaitResult == WAIT_ABANDONED) /* Mutex was not owned, we acquired it */
		return mutex;
	else
		return NULL;
}


/* The window proc prototype */
static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


/* Register a window's WinAPI "class" */
static BOOL registerClass(HINSTANCE hInstance, char *className, WNDPROC procName){
	WNDCLASSEXA wc;
	BOOL result;

	wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = 0;
	wc.lpfnWndProc = procName;
	wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = className;
    wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), NULL, IMAGE_ICON, 16, 16, 0);

	result = RegisterClassExA(&wc);

	return result;
}




/* Make the constructors */
#define FIELD(type, name, val) INIT_FIELD(type, name, val)
#define DEF_FIELD(type, name) DEFINE_FIELD(type, name, )
#define VIRTUAL_METHOD(classType, type, name, args) INIT_VIRTUAL_METHOD(classType, type, name, args)


/* Class Object */
/* The Constructor*/
void initObject(Object thisObject){
	if (!thisObject)
		return;
	
	CLASS_Object;
	
	/* Try to delete critical section just in case */
	if (thisObject->criticalSectionInitialized)
		DeleteCriticalSection(&(thisObject->criticalSection));

	InitializeCriticalSection(&(thisObject->criticalSection));
	thisObject->criticalSectionInitialized = TRUE;
}

Object newObject(){
	Object thisObject = (Object)malloc(sizeof(val_Object));

	if (!thisObject)
		return NULL;

	initObject(thisObject);

	return thisObject;
}

void deleteObject(Object thisObject){
	DeleteCriticalSection(&(thisObject->criticalSection));
	free(thisObject);
}





/* Class GUIObject */

BOOL GUIObject_addChild(GUIObject object, GUIObject child){
	GUIObject *newChildrenPointer;
	unsigned int i;
	
	if (!object)
		return FALSE;
	
	child->parent = object;
	
	if (object->children != NULL)
		for (i = 0; i < object->numChildren; i++)
			if ((object->children)[i] == NULL){
				(object->children)[i] = child;
				return TRUE;
			}
	
	
	(object->numChildren)++;
	newChildrenPointer = (GUIObject*)realloc(object->children, object->numChildren * sizeof(GUIObject));
	
	if (!newChildrenPointer)
		return FALSE;
	
	object->children = newChildrenPointer;
	(object->children)[object->numChildren - 1] = child;
	
	return TRUE;
}
	
/* Removes a child object from a GUIObject */
BOOL GUIObject_removeChild(GUIObject object, GUIObject child){
	unsigned int i;
	
	if (!object)
		return FALSE;
	
	child->parent = NULL;
	
	if (object->children != NULL)
		for (i = 0; i < object->numChildren; i++)
			if ((object->children)[i] == child){
				(object->children)[i] = NULL;
				return TRUE;
			}
	
	return FALSE;
}
	
/* Sets an event for a GUIObject by a Windows message */
int GUIObject_setEvent(GUIObject object, DWORD message, void(*callback)(GUIObject, void*, EventArgs),
						void *context, enum _syncMode mode){
	struct _event *tempReallocPointer;
	unsigned int i;
	
	if (!object)
		return -1;
	
	for (i = 0; i < object->numEvents; i++)
		if  ((object->events)[i].message == message){
			(object->events)[i].eventFunction = callback; (object->events)[i].mode = mode;
			(object->events)[i].sender = object; (object->events)[i].context = context;
			(object->events)[i].condition = NULL; (object->events)[i].interrupt = FALSE;
			(object->events)[i].enabled = TRUE;
			return (int)i;
		}
	
	(object->numEvents)++;
	tempReallocPointer = (struct _event*)realloc(object->events, object->numEvents * sizeof(struct _event));
	if (!tempReallocPointer)
		return -1;
	
	object->events = tempReallocPointer;
	switch(message){
	case WM_MOUSEMOVE: case WM_MOUSEHOVER: case WM_MOUSELEAVE: case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_LBUTTONUP: 
	case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
			(object->events)[i].args = (EventArgs)newMouseEventArgs(message, 0, 0);
			break;
		default:
			(object->events)[i].args = newEventArgs(message, 0, 0);
	}
	
	(object->events)[object->numEvents - 1].eventFunction = callback; (object->events)[object->numEvents - 1].mode = mode;
	(object->events)[object->numEvents - 1].message = message; (object->events)[object->numEvents - 1].sender = object;
	(object->events)[object->numEvents - 1].context = context; (object->events)[object->numEvents - 1].condition = NULL;
	(object->events)[object->numEvents - 1].interrupt = FALSE; (object->events)[object->numEvents - 1].enabled = TRUE;
	
	return (int)(object->numEvents - 1);
}
	
/* Sets a condition for an event of a GUIObject */
BOOL GUIObject_setEventCondition(GUIObject object, int eventID, BOOL *condition){
	if (!object || eventID < 0 || (UINT)eventID >= object->numEvents)
		return FALSE;
	
	(object->events)[eventID].condition = condition;
	return TRUE;
}
	
/* Sets an interrupt state for an event of a GUIObject */
BOOL GUIObject_setEventInterrupt(GUIObject object, int eventID, BOOL interrupt){
	if (!object || eventID < 0 || (UINT)eventID >= object->numEvents)
		return FALSE;
	
	(object->events)[eventID].interrupt = interrupt;
	return TRUE;
}
	
/* Changes an event's enabled state for a GUIObject */
BOOL GUIObject_setEventEnabled(GUIObject object, int eventID, BOOL enabled){
	if (!object || eventID < 0 || (UINT)eventID >= object->numEvents)
		return FALSE;
	
	(object->events)[eventID].enabled = enabled;
	return TRUE;
}
	
/* Sets a WM_LBUTTONUP event for an (enabled) object */
int GUIObject_setOnClick(GUIObject object, void(*callback)(GUIObject, void*, EventArgs), void *context, enum _syncMode mode){
	int eventID = GUIObject_setEvent(object, WM_LBUTTONUP, callback, context, mode);
	return GUIObject_setEventCondition(object, eventID, &(object->enabled)) ? eventID : -1;
}
	
/* Moves a GUIObject to a new location specified by x and y */
BOOL GUIObject_setPos(GUIObject object, int x, int y){
	if (!object)
		return FALSE;
	
	object->x = x; object->realX = x;
	object->y = y; object->realY = y;
	if (!SetWindowPos(object->handle, NULL, x, y, object->width, object->height, SWP_NOSIZE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;
	
	if (!InvalidateRect(object->handle, NULL, FALSE))
		return FALSE;
	
	return TRUE;
}
	
/* Resizes a GUIObject to a new size specified by width and height */
BOOL GUIObject_setSize(GUIObject object, int width, int height){
	if (!object)
		return FALSE;
	
	object->realWidth = width;
	object->realHeight = height;
	
	if (width >= object->minWidth && width <= object->maxWidth)
		object->width = width;
	if (height >= object->minHeight && height <= object->maxHeight)
		object->height = height;
	
	if (!SetWindowPos(object->handle, NULL, object->x, object->y, object->width, object->height, SWP_NOMOVE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;
	
	if (!InvalidateRect(object->handle, NULL, FALSE))
		return FALSE;
	
	return TRUE;
}
	
/* Sets a GUIObject's minimum size to a new value specified by minWidth and minHeight */
BOOL GUIObject_setMinSize(GUIObject object, int minWidth, int minHeight){
	if (!object)
		return FALSE;
	
	object->minWidth = minWidth;
	object->minHeight = minHeight;
	
	if (minWidth > object->width){
		object->width = minWidth;
		object->realWidth = minWidth;
	}
	
	if (minHeight > object->height){
		object->height = minHeight;
		object->realHeight = minHeight;
	}
	
	if (!SetWindowPos(object->handle, NULL, object->x, object->y, object->width, object->height, SWP_NOMOVE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;
	
	if (!InvalidateRect(object->handle, NULL, FALSE))
		return FALSE;
	
	return TRUE;
}
	
/* Sets a GUIObject's maximum size to a new value specified by maxWidth and maxHeight */
BOOL GUIObject_setMaxSize(GUIObject object, int maxWidth, int maxHeight){
	if (!object)
		return FALSE;
	
	object->maxWidth = maxWidth;
	object->maxHeight = maxHeight;
	
	if (maxWidth < object->width){
		object->width = maxWidth;
		object->realWidth = maxWidth;
	}
	
	if (maxHeight < object->height){
		object->height = maxHeight;
		object->realHeight = maxHeight;
	}
	
	if (!SetWindowPos(object->handle, NULL, object->x, object->y, object->width, object->height, SWP_NOMOVE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;
	
	if (!InvalidateRect(object->handle, NULL, FALSE))
		return FALSE;
	
	return TRUE;
}
	
/* Sets a new text for a GUIObject */
BOOL GUIObject_setText(GUIObject object, char *text){
	int textLength = strlen(text);
	char *tempReallocPointer;
		
	if (!object)
		return FALSE;
	
	tempReallocPointer = (char*)realloc(object->text, textLength + 1);
	
	if (!tempReallocPointer)
		return FALSE;
	strcpy_s(tempReallocPointer, textLength + 1, text);
	object->text = tempReallocPointer;
	if (object->handle)
		return SetWindowTextA(object->handle, text);
	else
		return TRUE;
}
	
/* Sets a GUIObject's enabled state */
BOOL GUIObject_setEnabled(GUIObject object, BOOL enabled){
	if (!object)
		return FALSE;
	object->enabled = enabled;
	
	if (object->handle)
		EnableWindow(object->handle, enabled);
	return TRUE;
}

PRIVATE HBITMAP GUIObject_updateOffscreenPaintContext(GUIObject object, BOOL eraseBG, BOOL transparent){
	HBITMAP prevBitmap;
	RECT clientRect;
	HBRUSH backgroundBrush;

	GetClientRect(object->handle, &clientRect);

	if (!object->offscreenBitmap){
		if (object->offscreenPaintContext)
			DeleteObject(object->offscreenPaintContext);

		object->offscreenPaintContext = CreateCompatibleDC(object->paintContext);
		object->offscreenBitmap = CreateCompatibleBitmap(object->paintContext, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
	}

	prevBitmap = SelectBitmap(object->offscreenPaintContext, object->offscreenBitmap);

	if (eraseBG){
		backgroundBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
		FillRect(object->offscreenPaintContext, &clientRect, backgroundBrush);
		DeleteObject(backgroundBrush);
	}

	if (transparent)
		SetBkMode(object->offscreenPaintContext, TRANSPARENT);

	return prevBitmap;
}

PRIVATE void GUIObject_updatePaintContext(GUIObject object, HBITMAP prevBitmap){
	RECT clientRect;
	
	GetClientRect(object->handle, &clientRect);

	BitBlt(object->paintContext, clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
		object->offscreenPaintContext, 0, 0, SRCCOPY);
	SelectObject(object->offscreenPaintContext, prevBitmap);
}
	
/* Draws a line in a GUIObject from the point specified by x1, y1 to the point specified by x2, y2 */
BOOL GUIObject_drawLine(GUIObject object, Pen pen, int x1, int y1, int x2, int y2){
	HPEN prevPen = NULL, activePen;
	POINT prevPoint;
	BOOL acquiredContext = FALSE;
	HBITMAP prevBitmap;

	if (!object)
		return FALSE;

	if (!pen)
		activePen = GetStockPen(NULL_PEN); /* If no pen was passed, use a null pen */
	else
		activePen = pen->handle;

	
	if (!object->paintContext){
		object->paintContext = GetDC(object->handle);
		acquiredContext = TRUE;
	}

	prevBitmap = GUIObject_updateOffscreenPaintContext(object, TRUE, TRUE);

	prevPen = (HPEN)SelectObject(object->offscreenPaintContext, activePen);
	if (!prevPen || !MoveToEx(object->offscreenPaintContext, x1, y1, &prevPoint))
		goto release_DC_on_error;
	if (!LineTo(object->offscreenPaintContext, x2, y2))
		goto release_DC_on_error;
	/* Restore previous values */
	if (!MoveToEx(object->offscreenPaintContext, prevPoint.x, prevPoint.y, &prevPoint) || !SelectObject(object->offscreenPaintContext, prevPen))
		goto release_DC_on_error;

	GUIObject_updatePaintContext(object, prevBitmap);
	
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}

	return TRUE;

	release_DC_on_error:
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}
	return FALSE;
}
	
/* Draws an arc in a GUIObject */
BOOL GUIObject_drawArc(GUIObject object, Pen pen, int boundX1, int boundY1, int boundX2, int boundY2, int x1, int y1, int x2, int y2){
	HPEN prevPen = NULL, activePen;
	BOOL acquiredContext = FALSE;
	HBITMAP prevBitmap;

	if (!object)
		return FALSE;

	if (!pen)
		activePen = GetStockPen(NULL_PEN); /* If no pen was passed, use a null pen */
	else
		activePen = pen->handle;

	
	if (!object->paintContext){
		object->paintContext = GetDC(object->handle);
		acquiredContext = TRUE;
	}

	prevBitmap = GUIObject_updateOffscreenPaintContext(object, TRUE, TRUE);
	
	prevPen = (HPEN)SelectObject(object->offscreenPaintContext, activePen);
	if (!prevPen)
		goto release_DC_on_error;
	if (!Arc(object->offscreenPaintContext, boundX1, boundY1, boundX2, boundY2, x1, y1, x2, y2))
		goto release_DC_on_error;
	/* Restore previous values */
	if (!SelectObject(object->offscreenPaintContext, prevPen))
		goto release_DC_on_error;
	
	GUIObject_updatePaintContext(object, prevBitmap);
	
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}

	return TRUE;

	release_DC_on_error:
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}
	return FALSE;
}
	
/* Draws a rectangle in a GUIObject */
BOOL GUIObject_drawRect(GUIObject object, Pen pen, Brush brush, int boundX1, int boundY1, int boundX2, int boundY2){
	HPEN prevPen = NULL, activePen;
	HBRUSH prevBrush = NULL, activeBrush;
	BOOL acquiredContext = FALSE;
	HBITMAP prevBitmap;

	if (!object)
		return FALSE;

	if (!pen)
		activePen = GetStockPen(NULL_PEN); /* If no pen was passed, use a null pen */
	else
		activePen = pen->handle;

	if (!brush)
		activeBrush = GetStockBrush(HOLLOW_BRUSH); /* If no brush was passed, use a hollow brush */
	else
		activeBrush = brush->handle;
	
	if (!object->paintContext){
		object->paintContext = GetDC(object->handle);
		acquiredContext = TRUE;
	}

	prevBitmap = GUIObject_updateOffscreenPaintContext(object, TRUE, TRUE);
	
	prevPen = (HPEN)SelectObject(object->offscreenPaintContext, activePen);
	prevBrush = (HBRUSH)SelectObject(object->offscreenPaintContext, activeBrush);
	if (!prevPen || !prevBrush)
		goto release_DC_on_error;
	if (!Rectangle(object->offscreenPaintContext, boundX1, boundY1, boundX2, boundY2))
		goto release_DC_on_error;
	/* Restore previous values */
	if (!SelectObject(object->offscreenPaintContext, prevPen) || !SelectObject(object->offscreenPaintContext, prevBrush))
		goto release_DC_on_error;

	GUIObject_updatePaintContext(object, prevBitmap);
	
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}

	return TRUE;

	release_DC_on_error:
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}
	return FALSE;
}
	
/* Draws a rounded rectangle in a GUIObject */
BOOL GUIObject_drawRoundedRect(GUIObject object, Pen pen, Brush brush, int boundX1, int boundY1, 
							int boundX2, int boundY2, int ellipseWidth, int ellipseHeight){
	HPEN prevPen = NULL, activePen;
	HBRUSH prevBrush = NULL;
	HBRUSH activeBrush;
	BOOL acquiredContext = FALSE;
	HBITMAP prevBitmap;

	if (!object)
		return FALSE;

	if (!pen)
		activePen = GetStockPen(NULL_PEN); /* If no pen was passed, use a null pen */
	else
		activePen = pen->handle;

	if (!brush)
		activeBrush = GetStockBrush(HOLLOW_BRUSH); /* If no brush was passed, use a hollow brush */
	else
		activeBrush = brush->handle;
	
	if (!object->paintContext){
		object->paintContext = GetDC(object->handle);
		acquiredContext = TRUE;
	}

	prevBitmap = GUIObject_updateOffscreenPaintContext(object, TRUE, TRUE);
	
	prevPen = (HPEN)SelectObject(object->offscreenPaintContext, activePen);
	prevBrush = (HBRUSH)SelectObject(object->offscreenPaintContext, activeBrush);
	if (!prevPen || !prevBrush)
		goto release_DC_on_error;
	if (!RoundRect(object->offscreenPaintContext, boundX1, boundY1, boundX2, boundY2, ellipseWidth, ellipseHeight))
		goto release_DC_on_error;
	/* Restore previous values */
	if (!SelectObject(object->offscreenPaintContext, prevPen) || !SelectObject(object->offscreenPaintContext, prevBrush))
		goto release_DC_on_error;

	GUIObject_updatePaintContext(object, prevBitmap);
	
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}

	return TRUE;

	release_DC_on_error:
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}
	return FALSE;
}
	
/* Draws an ellipse in a GUIObject */
BOOL GUIObject_drawEllipse(GUIObject object, Pen pen, Brush brush, int boundX1, int boundY1, int boundX2, int boundY2){
	HPEN prevPen = NULL, activePen;
	HBRUSH prevBrush = NULL;
	HBRUSH activeBrush;
	BOOL acquiredContext = FALSE;
	HBITMAP prevBitmap;

	if (!object)
		return FALSE;

	if (!pen)
		activePen = GetStockPen(NULL_PEN); /* If no pen was passed, use a null pen */
	else
		activePen = pen->handle;

	if (!brush)
		activeBrush = GetStockBrush(HOLLOW_BRUSH); /* If no brush was passed, use a hollow brush */
	else
		activeBrush = brush->handle;
	
	if (!object->paintContext){
		object->paintContext = GetDC(object->handle);
		acquiredContext = TRUE;
	}

	prevBitmap = GUIObject_updateOffscreenPaintContext(object, TRUE, TRUE);
	
	prevPen = (HPEN)SelectObject(object->offscreenPaintContext, activePen);
	prevBrush = (HBRUSH)SelectObject(object->offscreenPaintContext, activeBrush);
	if (!prevPen || !prevBrush)
		goto release_DC_on_error;
	if (!Ellipse(object->offscreenPaintContext, boundX1, boundY1, boundX2, boundY2))
		goto release_DC_on_error;
	/* Restore previous values */
	if (!SelectObject(object->offscreenPaintContext, prevPen) || !SelectObject(object->offscreenPaintContext, prevBrush))
		goto release_DC_on_error;

	GUIObject_updatePaintContext(object, prevBitmap);
	
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}

	return TRUE;

	release_DC_on_error:
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}
	return FALSE;
}
	
/* Draws a polygon in a GUIObject */
BOOL GUIObject_drawPolygon(GUIObject object, Pen pen, Brush brush, int numPoints, LONG *coords){
	HPEN prevPen = NULL, activePen;
	HBRUSH prevBrush = NULL;
	HBRUSH activeBrush;
	POINT *points = (POINT*)malloc(numPoints * sizeof(POINT));
	BOOL acquiredContext = FALSE;
	HBITMAP prevBitmap;

	int i;
	
	if (!object || numPoints < 2){
		free(points);
		return FALSE;
	}

	if (!pen)
		activePen = GetStockPen(NULL_PEN); /* If no pen was passed, use a null pen */
	else
		activePen = pen->handle;

	if (!brush)
		activeBrush = GetStockBrush(HOLLOW_BRUSH); /* If no brush was passed, use a hollow brush */
	else
		activeBrush = brush->handle;
	
	if (!object->paintContext){
		object->paintContext = GetDC(object->handle);
		acquiredContext = TRUE;
	}
	
	for (i = 0; i < numPoints * 2; i+=2){
		points[i / 2].x = coords[i];
		points[i / 2].y = coords[i + 1];
	}

	prevBitmap = GUIObject_updateOffscreenPaintContext(object, TRUE, TRUE);
	
	prevPen = (HPEN)SelectObject(object->offscreenPaintContext, activePen);
	prevBrush = (HBRUSH)SelectObject(object->offscreenPaintContext, activeBrush);
	if (!prevPen || !prevBrush)
		goto release_DC_on_error;
	if (!Polygon(object->offscreenPaintContext, points, numPoints))
		goto release_DC_on_error;
	/* Restore previous values */
	if (!SelectObject(object->offscreenPaintContext, prevPen) || !SelectObject(object->offscreenPaintContext, prevBrush))
		goto release_DC_on_error;

	GUIObject_updatePaintContext(object, prevBitmap);
	
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}

	free(points);
	return TRUE;

	release_DC_on_error:
	if (acquiredContext){
		ReleaseDC(object->handle, object->paintContext);
		object->paintContext = NULL;
	}

	free(points);
	return FALSE;
}

/* Get the number of symbols in the decimal representation of a number. */
PRIVATE unsigned int getNumLength(unsigned int num){
    unsigned int numLength = 1;
    while ((num /= 10) > 0)
        numLength++;
    return numLength;
}

PRIVATE void setGUIObjectFields(GUIObject thisObject, HINSTANCE instance, char *text, int width, int height){
	static HMENU ID = NULL;
	unsigned int IDLength, textLength;
	
	/* Set the fields */

	thisObject->moduleInstance = instance;

	initObject((Object)thisObject);


	EnterCriticalSection(&(thisObject->criticalSection));
	thisObject->ID = ID;
	ID++;
	LeaveCriticalSection(&(thisObject->criticalSection));

	IDLength = getNumLength((unsigned int)(thisObject->ID));
	thisObject->className = (char*)malloc(IDLength + 1);
	if (!thisObject->className)
		goto alloc_className_failed;
	sprintf_s(thisObject->className, IDLength + 1, "%d", thisObject->ID);

	thisObject->numEvents = 1;
	thisObject->events = (struct _event*)malloc(thisObject->numEvents * sizeof(struct _event)); //TODO: add more events
	if (!thisObject->events)
		goto alloc_events_failed;
	(thisObject->events)[0].eventFunction = NULL; (thisObject->events)[0].mode = SYNC; (thisObject->events)[0].message = WM_LBUTTONUP;
	(thisObject->events)[0].sender = thisObject; (thisObject->events)[0].context = NULL;
	(thisObject->events)[0].args = newEventArgs(WM_LBUTTONUP, 0, 0); (thisObject->events)[0].condition = NULL;
	(thisObject->events)[0].interrupt = FALSE; (thisObject->events)[0].enabled = FALSE;

	if (text){
		textLength = strlen(text);
		thisObject->text = (char*)malloc(textLength + 1);
		if (!thisObject->text)
			goto alloc_text_failed;
		strcpy_s(thisObject->text, textLength + 1, text); /* Copy the object text to the heap */
	} else
		thisObject->text = NULL;

	thisObject->width = width; thisObject->height = height;
	thisObject->realWidth = width; thisObject->realHeight = height;

	return;

	/* Cleanup in case of malloc failures. That's where goto's come in handy */
	alloc_text_failed: free(thisObject->events);
	alloc_events_failed: free(thisObject->className);
	alloc_className_failed: DeleteCriticalSection(&(thisObject->criticalSection));
}

/* The Constructors*/
void initGUIObject(GUIObject thisObject, HINSTANCE instance, char *text, int width, int height){

	if (!thisObject)
		return;

	/* Initialize the fields to the default values */
	CLASS_GUIObject;

	setGUIObjectFields(thisObject, instance, text, width, height);
	
	thisObject->type = GUIOBJECT;

	return;
}

GUIObject newGUIObject(HINSTANCE instance, char *text, int width, int height){
	GUIObject thisObject = (GUIObject)malloc(sizeof(val_GUIObject));
	if (!thisObject)
		return NULL;

	initGUIObject(thisObject, instance, text, width, height);
	return thisObject;
}

void freeGUIObjectFields(GUIObject object){
	unsigned int i;
	EndPaint(object->handle, &(object->paintData));
	if (!DestroyWindow(object->handle))
		SendMessageA(object->handle, WM_CLOSE, (WPARAM)NULL, (WPARAM)NULL);
	
	DeleteCriticalSection(&(object->criticalSection));

	if (object->parent)
		GUIObject_removeChild(object->parent, object);

	free(object->className);
	if (object->events){
		for (i = 0; i < object->numEvents; i++)
			if ((object->events)[i].args)
				if ((object->events)[i].args->type == EVENTARGS)
					deleteEventArgs((object->events)[i].args);
				else if ((object->events)[i].args->type == MOUSEEVENTARGS)
					deleteMouseEventArgs((MouseEventArgs)(object->events)[i].args);
		free(object->events);
	}
	free(object->text);
}

/* The Destructor*/
void deleteGUIObject(GUIObject object){
	freeGUIObjectFields(object);
	free(object);
}





/* Class Window */
/* The Methods*/
/* Changes a window's resizable style */
BOOL Window_setResizable(Window window, BOOL resizable){
	if (!window)
		return FALSE;
	window->resizable = resizable;
	window->styles = resizable ? (window->styles) | WS_THICKFRAME : (window->styles) ^ WS_THICKFRAME;
		
	if (window->handle)
		return SetWindowLongPtrA(window->handle, GWL_STYLE, (LONG)(window->styles)) ? TRUE : FALSE;
	else
		return TRUE;
}
	
/* Enables or disables a window's maximize box */
BOOL Window_enableMaximize(Window window, BOOL maximize){
	if (!window)
		return FALSE;
	window->maximizeEnabled = maximize;
	window->styles = maximize ? window->styles | WS_MAXIMIZEBOX : window->styles ^ WS_MAXIMIZEBOX;
		
	if (window->handle)
		return SetWindowLongPtrA(window->handle, GWL_STYLE, (LONG)(window->styles)) ? TRUE : FALSE;
	else
		return TRUE;
}


/* The Constructors*/
void initWindow(Window thisObject, HINSTANCE instance, char *text, int width, int height){
	if (!thisObject)
		return;

	CLASS_Window;

	setGUIObjectFields((GUIObject)thisObject, instance, text, width, height);

	/* Register the window class */
    if (!registerClass(thisObject->moduleInstance, thisObject->className, (WNDPROC)windowProc))
    	return;

	/* Set the fields */
	thisObject->type = WINDOW;
	thisObject->styles = WS_OVERLAPPEDWINDOW;
	thisObject->exStyles = WS_EX_WINDOWEDGE;
}

Window newWindow(HINSTANCE instance, char *text, int width, int height){
	Window thisObject = (Window)malloc(sizeof(val_Window));

	if (!thisObject)
		return NULL;

	initWindow(thisObject, instance, text, width, height);

	return thisObject;
}

/* The Destructor*/
void deleteWindow(Window window){
	freeGUIObjectFields((GUIObject)window);
	free(window);
}





/* Class Control */
/* The Methods*/
/* Moves a GUIObject to a new location specified by x and y */
/* Overrides setPosT in GUIObject */
BOOL Control_setPos(GUIObject object, int x, int y){
	if (!object)
		return FALSE;
	
	object->realX = x;
	object->realY = y;

	if (x >= ((Control)object)->minX && x <= ((Control)object)->maxX)
		object->x = x;
	if (y >= ((Control)object)->minY && y <= ((Control)object)->maxY)
		object->y = y;

	if (!SetWindowPos(object->handle, NULL, object->x, object->y, object->width, object->height, SWP_NOSIZE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;

	if (!InvalidateRect(object->handle, NULL, FALSE))
		return FALSE;

	return TRUE;
}

/* Sets a control's minimum position to a new value specified by minX and minY */
BOOL Control_setMinPos(Control object, int minX, int minY){
	if (!object)
		return FALSE;
	
	object->minX = minX;
	object->minY = minY;
	
	if (minX > object->x){
		object->x = minX;
		object->realX = minX;
	}
	
	if (minY > object->y){
		object->y = minY;
		object->realY = minY;
	}
	
	if (!SetWindowPos(object->handle, NULL, object->x, object->y, object->width, object->height, SWP_NOMOVE | SWP_ASYNCWINDOWPOS |
																					SWP_DRAWFRAME))
		return FALSE;
	
	if (!InvalidateRect(object->handle, NULL, FALSE))
		return FALSE;
	
	return TRUE;
}

/* Sets a control's maximum position to a new value specified by maxX and maxY */
BOOL Control_setMaxPos(Control object, int maxX, int maxY){
	if (!object)
		return FALSE;
	
	object->maxX = maxX;
	object->maxY = maxY;
	
	if (maxX < object->x){
		object->x = maxX;
		object->realX = maxX;
	}
	
	if (maxY < object->y){
		object->y = maxY;
		object->realY = maxY;
	}
	
	if (!SetWindowPos(object->handle, NULL, object->x, object->y, object->width, object->height, SWP_NOMOVE | SWP_ASYNCWINDOWPOS |
																					SWP_DRAWFRAME))
		return FALSE;
	
	if (!InvalidateRect(object->handle, NULL, FALSE))
		return FALSE;
	
	return TRUE;
}


PRIVATE void setControlFields(Control thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height){
	if (!thisObject)
		return;

	setGUIObjectFields((GUIObject)thisObject, instance, text, width, height);

	/* Set the fields */
	thisObject->x = x; thisObject->realX = x;
	thisObject->y = y; thisObject->realY = y;
	thisObject->exStyles = WS_EX_WINDOWEDGE;

	free(thisObject->className);
	thisObject->className = NULL;

	/* Override virtual methods */
	thisObject->setPos = &Control_setPos;
}


/* The Constructors*/
void initControl(Control thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height){
	if (!thisObject)
		return;

	CLASS_Control;

	setControlFields(thisObject, instance, text, x, y, width, height);

	thisObject->type = CONTROL;
}

Control newControl(HINSTANCE instance, char *text, int x, int y, int width, int height){
	Control thisObject = (Control)malloc(sizeof(val_Control));

	if (!thisObject)
		return NULL;
	
	initControl(thisObject, instance, text, width, x, y, height);

	return thisObject;
}

/* The Destructor*/
void deleteControl(Control control){
	freeGUIObjectFields((GUIObject)control);
	free(control);
}





/* Class Button */
/* The Constructors*/
void initButton(Button thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height){
	if (!thisObject)
		return;

	CLASS_Button;

	setControlFields((Control)thisObject, instance, text, x, y, width, height);

	/* Set the fields */
	thisObject->type = BUTTON;
	thisObject->className = "Button";
	thisObject->styles = WS_CHILD | WS_VISIBLE | BS_TEXT | BS_PUSHBUTTON;
}

Button newButton(HINSTANCE instance, char *text, int x, int y, int width, int height){
	Button thisObject = (Button)malloc(sizeof(val_Button));

	if (!thisObject)
		return NULL;

	initButton(thisObject, instance, text, x, y, width, height);

	return thisObject;
}

/* The Destructor*/
void deleteButton(Button button){
	button->className = NULL;
	freeGUIObjectFields((GUIObject)button);
	free(button);
}





/* Class TextBox */
/* The Methods*/
/* Sets the text input mode for a textbox to number-only or to not number-only */
BOOL TextBox_setNumOnly(TextBox textbox, BOOL numOnly){
	textbox->numOnly = numOnly;
	textbox->styles = numOnly ? textbox->styles | ES_NUMBER : textbox->styles ^ ES_NUMBER;
	
	if (textbox->handle)
		return SetWindowLongPtrA(textbox->handle, GWL_STYLE, (LONG)textbox->styles) ? TRUE : FALSE;
	else
		return TRUE;
}



/* The Constructors*/
void initTextBox(TextBox thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height, enum _textboxtype multiline){
	if (!thisObject)
		return;

	CLASS_TextBox;

	setControlFields((Control)thisObject, instance, text, x, y, width, height);

	/* Set the fields */
	thisObject->type = TEXTBOX;
	thisObject->className = "Edit";
	thisObject->styles = WS_CHILD | WS_VISIBLE | WS_BORDER;

	thisObject->multiline = (BOOL)multiline;
	if (thisObject->multiline)
		thisObject->styles |= ES_MULTILINE | ES_WANTRETURN;
}

TextBox newTextBox(HINSTANCE instance, char *text, int x, int y, int width, int height, enum _textboxtype multiline){
	TextBox thisObject = (TextBox)malloc(sizeof(val_TextBox));

	if (!thisObject)
		return NULL;

	initTextBox(thisObject, instance, text, x, y, width, height, multiline);

	return thisObject;
}

/* The Destructor*/
void deleteTextBox(TextBox textbox){
	textbox->className = NULL;
	freeGUIObjectFields((GUIObject)textbox);
	free(textbox);
}





/* Class Label */
/* The Constructors*/
void initLabel(Label thisObject, HINSTANCE instance, char *text, int x, int y, int width, int height){
	if (!thisObject)
		return;

	CLASS_Button;

	setControlFields((Control)thisObject, instance, text, x, y, width, height);

	/* Set the fields */
	thisObject->type = LABEL;
	thisObject->className = "Static";
	thisObject->styles = WS_CHILD | WS_VISIBLE | SS_NOTIFY;
}

Label newLabel(HINSTANCE instance, char *text, int x, int y, int width, int height){
	Label thisObject = (Label)malloc(sizeof(val_Label));

	if (!thisObject)
		return NULL;

	initLabel(thisObject, instance, text, x, y, width, height);

	return thisObject;
}

/* The Destructor*/
void deleteLabel(Label label){
	label->className = NULL;
	freeGUIObjectFields((GUIObject)label);
	free(label);
}





/* Class EventArgs */

/* The Constructors*/
void initEventArgs(EventArgs thisObject, UINT message, WPARAM wParam, LPARAM lParam){
	if (!thisObject)
		return;

	initObject((Object)thisObject);

	thisObject->type = EVENTARGS;

	thisObject->message = message;
	thisObject->wParam = wParam;
	thisObject->lParam = lParam;

	thisObject->updateValue = &initEventArgs;
}

EventArgs newEventArgs(UINT message, WPARAM wParam, LPARAM lParam){
	EventArgs thisObject = (EventArgs)malloc(sizeof(val_EventArgs));

	if (!thisObject)
		return NULL;

	initEventArgs(thisObject, message, wParam, lParam);

	return thisObject;
}

/* The Destructor*/
void deleteEventArgs(EventArgs eventargs){
	DeleteCriticalSection(&(eventargs->criticalSection));
	free(eventargs);
}





/* Class MouseEventArgs */

void MouseEventArgs_updateValue(EventArgs thisObject, UINT message, WPARAM wParam, LPARAM lParam){
	initMouseEventArgs((MouseEventArgs)thisObject, message, wParam, lParam);
}

/* The Constructors*/
void initMouseEventArgs(MouseEventArgs thisObject, UINT message, WPARAM wParam, LPARAM lParam){
	if (!thisObject)
		return;

	initEventArgs((EventArgs)thisObject, message, wParam, lParam);

	thisObject->type = MOUSEEVENTARGS;

	thisObject->cursorX = GET_X_LPARAM(lParam);
	thisObject->cursorY = GET_Y_LPARAM(lParam);

	thisObject->updateValue = &MouseEventArgs_updateValue;
}

MouseEventArgs newMouseEventArgs(UINT message, WPARAM wParam, LPARAM lParam){
	MouseEventArgs thisObject = (MouseEventArgs)malloc(sizeof(val_MouseEventArgs));
	if (!thisObject)
		return NULL;

	initMouseEventArgs(thisObject, message, wParam, lParam);

	return thisObject;
}

/* The Destructor*/
void deleteMouseEventArgs(MouseEventArgs mouseeventargs){
	DeleteCriticalSection(&(mouseeventargs->criticalSection));
	free(mouseeventargs);
}





/* Class Pen */

/* The Constructors*/
void initPen(Pen thisObject, int penStyle, int width, COLORREF color){
	if (!thisObject)
		return;

	initObject((Object)thisObject);

	thisObject->type = PEN;

	thisObject->penStyle = penStyle;
	thisObject->width = width;
	thisObject->color = color;

	if (thisObject->handle)
		DeleteObject(thisObject->handle);
	thisObject->handle = CreatePen(penStyle, width, color);
}

Pen newPen(int penStyle, int width, COLORREF color){
	Pen thisObject = (Pen)malloc(sizeof(val_Pen));

	if (!thisObject)
		return NULL;

	initPen(thisObject, penStyle, width, color);

	return thisObject;
}

/* The Destructor*/
void deletePen(Pen pen){
	DeleteObject(pen->handle);
	DeleteCriticalSection(&(pen->criticalSection));
	free(pen);
}





/* Class Brush */

/* The Constructors*/
void initBrush(Brush thisObject, UINT brushStyle, COLORREF color, ULONG_PTR hatch){
	LOGBRUSH brushInfo;

	if (!thisObject)
		return;

	initObject((Object)thisObject);

	thisObject->type = BRUSH;

	thisObject->brushStyle = brushStyle; brushInfo.lbStyle = brushStyle;
	thisObject->color = color; brushInfo.lbColor = color;
	thisObject->hatch = hatch; brushInfo.lbHatch = hatch;

	if (thisObject->handle)
		DeleteObject(thisObject->handle);

	thisObject->handle = CreateBrushIndirect(&brushInfo);
}

Brush newBrush(UINT brushStyle, COLORREF color, ULONG_PTR hatch){
	Brush thisObject = (Brush)malloc(sizeof(val_Brush));

	if (!thisObject)
		return NULL;

	initBrush(thisObject, brushStyle, color, hatch);

	return thisObject;
}

/* The Destructor*/
void deleteBrush(Brush brush){
	DeleteObject(brush->handle);
	DeleteCriticalSection(&(brush->criticalSection));
	free(brush);
}





/* WinAPI Call Functions */

/* Event handling */
/* The common asynchronous event callback function */
static DWORD WINAPI asyncEventProc(LPVOID event){
	struct _event *eventPointer = (struct _event*)event;
	eventPointer->eventFunction(eventPointer->sender, eventPointer->context, eventPointer->args);
	return TRUE;
}

/* Find and fire off an event for a GUIObject */
static int handleEvents(GUIObject currObject, UINT messageID, WPARAM wParam, LPARAM lParam){
	unsigned int i;
	BOOL condition = TRUE;

	for (i = 0; i < currObject->numEvents; i++){
		if  ((currObject->events)[i].message == messageID && (currObject->events)[i].eventFunction != NULL){
			if ((currObject->events)[i].condition)
				condition = *((currObject->events)[i].condition);

			if ((currObject->events)[i].enabled && condition && (currObject->events)[i].eventFunction){
				EnterCriticalSection(&((currObject->events)[i].args->criticalSection));
				(currObject->events)[i].args->updateValue((currObject->events)[i].args, messageID, wParam, lParam); /* Update the event args */
				LeaveCriticalSection(&((currObject->events)[i].args->criticalSection));

				if ((currObject->events)[i].mode == SYNC){
					((currObject->events)[i].eventFunction)((currObject->events)[i].sender,
															(currObject->events)[i].context, (currObject->events)[i].args);
				} else
					CreateThread(NULL, 0, asyncEventProc, (LPVOID)&(currObject->events)[i], 0, NULL);
			}

			return (int)i;
		}
	}

	return -1;
}

/* The event handling routine for WM_COMMAND type events for window controls */
static int commandEventHandler(HWND hwnd, WPARAM wParam, LPARAM lParam){
	UINT itemID = LOWORD(wParam), messageID = HIWORD(wParam);
	GUIObject currObject = (GUIObject)GetWindowLongPtrA((HWND)lParam, GWLP_USERDATA);
	char *controlText = NULL;
	int textLength = 0;
	
	if (currObject){
		if (messageID == EN_CHANGE){ /* Text in a textbox was changed by the user */
			textLength = GetWindowTextLengthA(currObject->handle);
			if (textLength > 0 && (controlText = (char*)malloc(textLength + 1))){
				if (GetWindowTextA(currObject->handle, controlText, textLength + 1) > 0){
					free(currObject->text);
					currObject->text = controlText;
				}
			}
		}

		return handleEvents(currObject, messageID, wParam, lParam);
	} else
		return -1;
}

/*FIELD VALUES CONSISTENCY SUPPORT ON REFRESH*/
/* Resizes and/or moves the children of a window or a control according to their anchor settings */
BOOL alignChildren(GUIObject object, int widthChange, int heightChange){
	unsigned int i;
	Control currChild;
	int controlWidthChange = 0, controlHeightChange = 0;

	if (!object)
		return FALSE;

	for (i = 0; i < object->numChildren; i++){
		if ((object->children)[i] != NULL){
			currChild = (Control)(object->children)[i];
			InvalidateRect(currChild->handle, NULL, FALSE);

			if (((currChild->anchor & 0xF000) && (currChild->anchor & 0x000F)) ||
					((currChild->anchor & 0x0F00) && (currChild->anchor & 0x00F0))){ /* Anchored top and bottom and/or left and right */
				if ((currChild->anchor & 0xF000) && (currChild->anchor & 0x000F)) /* Anchored left and right */
					controlWidthChange = widthChange;

				if ((currChild->anchor & 0x0F00) && (currChild->anchor & 0x00F0)) /* Anchored top and bottom */
					controlHeightChange = heightChange;

				EnterCriticalSection(&(currChild->criticalSection));
				GUIObject_setSize((object->children)[i], currChild->realWidth + controlWidthChange, currChild->realHeight + controlHeightChange);
				LeaveCriticalSection(&(currChild->criticalSection));

				if (currChild->numChildren != 0)
					if (!alignChildren((object->children)[i], controlWidthChange, controlHeightChange))
						return FALSE;
			} 
			
			if ((!(currChild->anchor & 0xF000) && (currChild->anchor & 0x000F)) || 
						(!(currChild->anchor & 0x0F00) && (currChild->anchor & 0x00F0))){ /* Anchored right but not left and/or bottom but not top */
				EnterCriticalSection(&(currChild->criticalSection));
				Control_setPos((object->children)[i], currChild->realX + widthChange, currChild->realY + heightChange);
				LeaveCriticalSection(&(currChild->criticalSection)); 
			}

			/* Not anchored left or right */
			if (!(currChild->anchor & 0xF000) && !(currChild->anchor & 0x000F)){
				EnterCriticalSection(&(currChild->criticalSection)); EnterCriticalSection(&((currChild->parent)->criticalSection));
				Control_setPos((object->children)[i], currChild->realX + (currChild->parent)->width / 2 - ((currChild->parent)->width - widthChange) / 2, 
							currChild->realY);
				LeaveCriticalSection(&(currChild->criticalSection)); LeaveCriticalSection(&((currChild->parent)->criticalSection));
			}

			/* Not anchored top or bottom */
			if  (!(currChild->anchor & 0x0F00) && !(currChild->anchor & 0x00F0)) {
				EnterCriticalSection(&(currChild->criticalSection)); EnterCriticalSection(&((currChild->parent)->criticalSection));
				Control_setPos((object->children)[i], currChild->realX, 
							currChild->realY + (currChild->parent)->height / 2 - ((currChild->parent)->height - heightChange) / 2);
				LeaveCriticalSection(&(currChild->criticalSection)); LeaveCriticalSection(&((currChild->parent)->criticalSection));
			}
		}
	}

	return TRUE;
}

/* Sets the current window's width, height and clientWidth and clientHeight on user resize. Thread-safe */
static void refreshWindowSize(Window window, LPARAM lParam){
	RECT clientSize;
	WINDOWPOS *windowPos;
	int widthChange = 0, heightChange = 0;

	EnterCriticalSection(&(window->criticalSection));

	windowPos = (WINDOWPOS*)lParam;
	window->width = windowPos->cx;
	window->height = windowPos->cy;
	window->x = windowPos->x;
	window->y = windowPos->y;

	if (GetClientRect(window->handle, &clientSize)){
		widthChange = clientSize.right - clientSize.left - window->clientWidth;
		heightChange = clientSize.bottom - clientSize.top - window->clientHeight;

		window->clientWidth = clientSize.right - clientSize.left;
		window->clientHeight = clientSize.bottom - clientSize.top;
	}

	InvalidateRect(window->handle, NULL, FALSE);

	LeaveCriticalSection(&(window->criticalSection));

	alignChildren((GUIObject)window, widthChange, heightChange);
}

/* Display a control on a window */
BOOL displayControl(Control control){
	HFONT hFont;
	LOGFONT lf;

	/* Add the control */
	control->handle = CreateWindowExA(control->exStyles, control->className, control->text, control->styles, control->x, control->y,
		control->width, control->height, (control->parent) ? (control->parent)->handle : NULL, control->ID, control->moduleInstance, NULL);

	if (!control->handle)
		return FALSE;

	/* Change its font */
	GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf); 
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);//CreateFontW(0, 0, 0, 0, 400, 0, 0, lf.lfStrikeOut, lf.lfCharSet, lf.lfOutPrecision, 
							//lf.lfClipPrecision, lf.lfQuality, lf.lfPitchAndFamily, lf.lfFaceName);
	//TODO: make the font a property of the Object type!
	SendMessageA(control->handle, WM_SETFONT, (WPARAM)hFont, TRUE);
		
	/* Set the control handle's additional data to a pointer to its object */
	SetWindowLongPtrA(control->handle, GWLP_USERDATA, (LONG)control);

	/* Subclass the control to make it send its messages through the main window proc */
	control->origProcPtr = SetWindowLongPtrA(control->handle, GWLP_WNDPROC, (LONG_PTR)windowProc);
	if (!control->origProcPtr)
		return FALSE;

	ShowWindow(control->handle, SW_SHOWDEFAULT);
	
    UpdateWindow(control->handle);
	EnableWindow(control->handle, control->enabled);
	if (control->parent)
		UpdateWindow(control->parent->handle);

	return TRUE;
}

/* Go through a GUIObject's list of children and add all of them to the window, then call itself on each of the children */
static void displayChildren(GUIObject object){
	unsigned int i;

	if (object)
		for (i = 0; i < object->numChildren; i++)
			if ((object->children)[i] != NULL){
				displayControl((Control)(object->children)[i]);
				displayChildren((object->children)[i]);
			}
}


/* The Window Procedure callback function */
static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	GUIObject currObject = NULL;
	int eventID = -1;
	BOOL interrupt = FALSE;
	LRESULT defCallResult = (LRESULT)NULL;

	if (hwnd != NULL)
		currObject = (GUIObject)GetWindowLongPtrA(hwnd, GWLP_USERDATA); /* Get the object that this handle belongs to */

	/* Default event handling */
	switch(msg){
		case WM_CREATE:
			if (currObject)
				displayChildren(currObject);
			break;

		case WM_PAINT:
			if (currObject){ /* Begin or end painting the object */
				/* We need default paint processing for the control to occur BEFORE we begin painting */
				if (currObject->type == WINDOW)
					defCallResult = DefWindowProcA(hwnd, msg, wParam, lParam);
				else
					defCallResult = CallWindowProcA((WNDPROC)(currObject->origProcPtr), hwnd, msg, wParam, lParam);

				if (!currObject->paintContext && currObject->handle)
					currObject->paintContext = GetDC(currObject->handle);
				if (!currObject->paintContext)
					currObject->paintContext = BeginPaint(currObject->handle, &(currObject->paintData));
			}
			break;

		case WM_ERASEBKGND:
			if (currObject->customEraseBG)
				return (LRESULT)TRUE;

		/*case WM_CTLCOLORSTATIC:
			return (LONG)GetStockObject(NULL_BRUSH);*/

		case WM_COMMAND:
			eventID = commandEventHandler(hwnd, wParam, lParam);
			break;

		case WM_NOTIFY:
			//TODO: add "notify" event handling!
			break;

		case WM_GETMINMAXINFO: /* The window size or position limits are queried */
			if (currObject){
				((MINMAXINFO*)lParam)->ptMaxSize.x = currObject->maxWidth;
				((MINMAXINFO*)lParam)->ptMaxSize.y = currObject->maxHeight;
				((MINMAXINFO*)lParam)->ptMaxTrackSize.x = currObject->maxWidth;
				((MINMAXINFO*)lParam)->ptMaxTrackSize.y = currObject->maxHeight;
				((MINMAXINFO*)lParam)->ptMinTrackSize.x = currObject->minWidth;
				((MINMAXINFO*)lParam)->ptMinTrackSize.y = currObject->minHeight;
			}
			break;

		case WM_WINDOWPOSCHANGED: /* The window size or position have just been changed */
			if (currObject->type == WINDOW)
				refreshWindowSize((Window)currObject, lParam);
			break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
			break;

        case WM_DESTROY:
			if (currObject && currObject->type == WINDOW)
				PostQuitMessage(0);
			break;
    }

	if (currObject){
		eventID = handleEvents(currObject, msg, wParam, lParam);
		
		if (msg == WM_PAINT && currObject->handle) {
			if (!ReleaseDC(currObject->handle, currObject->paintContext))
				EndPaint(currObject->handle, &(currObject->paintData));
			currObject->paintContext = NULL;

			return defCallResult;
		}
	}
	
	/* Call the window's events */
	if (currObject){
		if (eventID >= 0 && (UINT)eventID < currObject->numEvents)
			interrupt = (currObject->events)[eventID].interrupt;

		if (!interrupt){
			if (currObject->type == WINDOW)
				return DefWindowProcA(hwnd, msg, wParam, lParam);
			else /* Resend the messages to the subclassed object's default window proc */
				return CallWindowProcA((WNDPROC)(currObject->origProcPtr), hwnd, msg, wParam, lParam);
		}

		return 0;
	} else
		return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* Flushes the current thread's message queue */
void flushMessageQueue(){
	MSG msg;

	while (PeekMessageA(&msg,NULL,0,0,PM_REMOVE))
		GetMessageA(&msg, NULL, 0, 0);
}

/* Display a window with the application's command line settings */
BOOL displayWindow(Window mainWindow, int nCmdShow){
    MSG msg;
	RECT clientRect;

	mainWindow->handle = CreateWindowExA(mainWindow->exStyles,  mainWindow->className,
																mainWindow->text,
																mainWindow->styles,
																mainWindow->x, mainWindow->y, 
																mainWindow->width,
																mainWindow->height,
																NULL, NULL, 
																mainWindow->moduleInstance, NULL);

	if (!mainWindow->handle)
		return FALSE;

	/* Set the window handle's additional data to a pointer to its object */
	SetWindowLongPtrA(mainWindow->handle, GWLP_USERDATA, (LONG)(mainWindow));

	if (GetClientRect(mainWindow->handle, &clientRect)){
		mainWindow->clientWidth = clientRect.right - clientRect.left;
		mainWindow->clientHeight = clientRect.bottom - clientRect.top;
	}

	displayChildren((GUIObject)mainWindow);

	ShowWindow(mainWindow->handle, nCmdShow);
    UpdateWindow(mainWindow->handle);
	EnableWindow(mainWindow->handle, mainWindow->enabled);

    /* The message loop */
    while(GetMessageA(&msg, NULL, 0, 0) > 0){
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return msg.wParam;
}