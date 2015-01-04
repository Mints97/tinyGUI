#include "tinyGUI.h"

/* =====================================================Thread synchronization================================================================= */
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

/* =============================================THE SELF-REFERENCE MECHANISM=============================================== */

/* The Thread Local Storage index */
static DWORD tlsIndex = TLS_OUT_OF_INDEXES;

/* Returns the context for the current thread, or allocates a new context if it doesn't exist. Thread-safe */
static void *getCurrentThis(){
	return TlsGetValue(tlsIndex);
}

/* Sets the context for the current thread */
void setCurrentThis(void *self){
	HANDLE mutex;
	
	if (TlsGetValue(tlsIndex) != self){
		if (tlsIndex == TLS_OUT_OF_INDEXES){
			mutex = getMutexOwnership("Local\\tlsSet");
			if (!mutex)
				return;

			tlsIndex = TlsAlloc(); /* If the Thread Local Storage index hasn't been allocated, allocate it */

			if (tlsIndex == TLS_OUT_OF_INDEXES){
				ReleaseMutex(mutex);
				return;
			}

			if (!ReleaseMutex(mutex)) /* Release mutex ownership */
				return;
		}

		TlsSetValue(tlsIndex, self);
	}
}


/* ============================================OTHER======================================================================= */
/* Quickly get the number of symbols in the decimal representation of a number. */
static int getNumLength(unsigned int x) {
    if(x>=1000000000) return 10; if(x>=100000000) return 9; if(x>=10000000) return 8; if(x>=1000000) return 7;
    if(x>=100000) return 6; if(x>=10000) return 5; if(x>=1000) return 4; if(x>=100) return 3; if(x>=10) return 2;
    return 1;
}



/* ======================================================Class Object============================================================================= */
/* -------------------------------------------------The Constructor-------------------------------------------------------------------------------*/
struct _object *newObject(){
	struct _object *object = (struct _object*)malloc(sizeof(struct _object));

	if (!object)
		return NULL;

	/* Set the fields */
	object->criticalSection = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
	if (!object->criticalSection) return NULL; InitializeCriticalSection(object->criticalSection);

	safeInit(object->type, enum _objectType, GUIOBJECT);

	return object;
}

/* -------------------------------------------------The Destructor--------------------------------------------------------------------------------*/
void deleteObject(struct _object *object){
	if (object->criticalSection) DeleteCriticalSection(object->criticalSection);
	free(object->criticalSection);
	free(object->type);

	free(object);
}





/* ======================================================Class GUIObject=========================================================================== */
/* -------------------------------------------------The Methods---------------------------------------------------------------------------------*/
/* Adds a child object to a GUIObject */
static BOOL addChild(struct _guiobject *object, struct _guiobject *child){
	struct _guiobject **newChildrenPointer;
	unsigned int i;

	if (!object)
		return FALSE;

	*child->parent = object;

	if (*object->children != NULL)
		for (i = 0; i < *object->numChildren; i++)
			if ((*object->children)[i] == NULL){
				(*object->children)[i] = child;
				return TRUE;
			}


	(*object->numChildren)++;
	newChildrenPointer = (struct _guiobject**)realloc(*object->children, *object->numChildren * sizeof(struct _guiobject*));

	if (!newChildrenPointer)
		return FALSE;

	*object->children = newChildrenPointer;
	(*object->children)[*object->numChildren - 1] = child;

	return TRUE;
}

/* Adds a child object to the current object specified in the thread's context (the self-reference mechanism) */
static BOOL addChildSelfRef(struct _guiobject *child){
	return addChild((struct _guiobject*)getCurrentThis(), child);
}

/* Removes a child object from a GUIObject */
static BOOL removeChild(struct _guiobject *object, struct _guiobject *child){
	unsigned int i;

	if (!object)
		return FALSE;

	*child->parent = object;

	if (*object->children != NULL)
		for (i = 0; i < *object->numChildren; i++)
			if ((*object->children)[i] == child){
				(*object->children)[i] = NULL;
				return TRUE;
			}

	return FALSE;
}

/* Removes a child object from the current object specified in the thread's context (the self-reference mechanism) */
static BOOL removeChildSelfRef(struct _guiobject *child){
	return addChild((struct _guiobject*)getCurrentThis(), child);
}

/* Sets an event for a GUIObject by a Windows message */
static int setEvent(struct _guiobject *object, DWORD message, void(*callback)(struct _guiobject*, void*, struct _eventargs*),
					 void *context, enum _syncMode mode){
	struct _event *tempReallocPointer;
	unsigned int i;

	if (!object)
		return -1;

	for (i = 0; i < *object->numEvents; i++)
		if  ((*object->events)[i].message == message){
			(*object->events)[i].eventFunction = callback; (*object->events)[i].mode = mode;
			(*object->events)[i].sender = object; (*object->events)[i].context = context;
			(*object->events)[i].condition = NULL; (*object->events)[i].interrupt = FALSE;
			(*object->events)[i].enabled = TRUE;
			return (int)i;
		}

	(*object->numEvents)++;
	tempReallocPointer = (struct _event*)realloc(*object->events, *object->numEvents * sizeof(struct _event));
	if (!tempReallocPointer)
		return -1;

	*object->events = tempReallocPointer;
	switch(message){
	case WM_MOUSEMOVE: case WM_MOUSEHOVER: case WM_MOUSELEAVE: case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_LBUTTONUP: 
	case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
			(*object->events)[i].args = (struct _eventargs*)newMouseEventArgs(message, 0, 0);
			break;
		default:
			(*object->events)[i].args = newEventArgs(message, 0, 0);
	}

	(*object->events)[*object->numEvents - 1].eventFunction = callback; (*object->events)[*object->numEvents - 1].mode = mode;
	(*object->events)[*object->numEvents - 1].message = message; (*object->events)[*object->numEvents - 1].sender = object;
	(*object->events)[*object->numEvents - 1].context = context; (*object->events)[*object->numEvents - 1].condition = NULL;
	(*object->events)[*object->numEvents - 1].interrupt = FALSE; (*object->events)[*object->numEvents - 1].enabled = TRUE;

	return (int)(*object->numEvents - 1);
}

/* Sets an event for a GUIObject specified in the thread's context (the self-reference mechanism) on a Windows message */
static int setEventSelfRef(DWORD message, void(*callback)(struct _guiobject*, void*, struct _eventargs*), void *context, enum _syncMode mode){
	return setEvent((struct _guiobject*)getCurrentThis(), message, callback, context, mode);
}

/* Sets a condition for an event of a GUIObject */
static BOOL setEventCondition(struct _guiobject *object, int eventID, BOOL *condition){
	if (!object || eventID < 0 || (UINT)eventID >= *object->numEvents)
		return FALSE;

	(*object->events)[eventID].condition = condition;
	return TRUE;
}

/* Sets a condition for an event of a GUIObject specified in the thread's context (the self-reference mechanism) */
static BOOL setEventConditionSelfRef(int eventID, BOOL *condition){
	return setEventCondition((struct _guiobject*)getCurrentThis(), eventID, condition);
}

/* Sets an interrupt state for an event of a GUIObject */
static BOOL setEventInterrupt(struct _guiobject *object, int eventID, BOOL interrupt){
	if (!object || eventID < 0 || (UINT)eventID >= *object->numEvents)
		return FALSE;

	(*object->events)[eventID].interrupt = interrupt;
	return TRUE;
}

/* Sets an interrupt state for an event of a GUIObject specified in the thread's context (the self-reference mechanism) */
static BOOL setEventInterruptSelfRef(int eventID, BOOL interrupt){
	return setEventInterrupt((struct _guiobject*)getCurrentThis(), eventID, interrupt);
}

/* Changes an event's enabled state for a GUIObject */
static BOOL setEventEnabled(struct _guiobject *object, int eventID, BOOL enabled){
	if (!object || eventID < 0 || (UINT)eventID >= *object->numEvents)
		return FALSE;

	(*object->events)[eventID].enabled = enabled;
	return TRUE;
}

/* Changes an event's enabled state for a GUIObject specified in the thread's context (the self-reference mechanism) */
static BOOL setEventEnabledSelfRef(int eventID, BOOL enabled){
	return setEventEnabled((struct _guiobject*)getCurrentThis(), eventID, enabled);
}

/* Sets a WM_LBUTTONUP event for an (enabled) object */
static int setOnClick(struct _guiobject *object, void(*callback)(struct _guiobject*, void*, struct _eventargs*), void *context, enum _syncMode mode){
	int eventID = setEvent(object, WM_LBUTTONUP, callback, context, mode);
	return setEventCondition(object, eventID, object->enabled) ? eventID : -1;
}

/* Sets a WM_LBUTTONUP event for an (enabled) object specified in the thread's context (the self-reference mechanism) */
static int setOnClickSelfRef(void(*callback)(struct _guiobject*, void*, struct _eventargs*), void *context, enum _syncMode mode){
	return setOnClick((struct _guiobject*)getCurrentThis(), callback, context, mode);
}

/* Moves a GUIObject to a new location specified by x and y */
static BOOL setPos(struct _guiobject *object, int x, int y){
	if (!object)
		return FALSE;

	*object->x = x; *object->realX = x;
	*object->y = y; *object->realY = y;
	if (!SetWindowPos(*object->handle, NULL, x, y, *object->width, *object->height, SWP_NOSIZE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;

	if (!InvalidateRect(*object->handle, NULL, FALSE))
		return FALSE;

	return TRUE;
}

/* Moves a GUIObject specified in the thread's context (the self-reference mechanism) to a new location specified by x
   and y */
static BOOL setPosSelfRef(int x, int y){
	return setPos((struct _guiobject*)getCurrentThis(), x, y);
}

/* Resizes a GUIObject to a new size specified by width and height */
static BOOL setSize(struct _guiobject *object, int width, int height){
	if (!object)
		return FALSE;

	*object->realWidth = width;
	*object->realHeight = height;

	if (width >= *object->minWidth && width <= *object->maxWidth)
		*object->width = width;
	if (height >= *object->minHeight && height <= *object->maxHeight)
		*object->height = height;

	if (!SetWindowPos(*object->handle, NULL, *object->x, *object->y, *object->width, *object->height, SWP_NOMOVE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;

	if (!InvalidateRect(*object->handle, NULL, FALSE))
		return FALSE;

	return TRUE;
}

/* Resizes a GUIObject specified in the thread's context (the self-reference mechanism) to a new size specified by width
   and height */
static BOOL setSizeSelfRef(int width, int height){
	return setSize((struct _guiobject*)getCurrentThis(), width, height);
}

/* Sets a GUIObject's minimum size to a new value specified by minWidth and minHeight */
static BOOL setMinSize(struct _guiobject *object, int minWidth, int minHeight){
	if (!object)
		return FALSE;

	*object->minWidth = minWidth;
	*object->minHeight = minHeight;

	if (minWidth > *object->width){
		*object->width = minWidth;
		*object->realWidth = minWidth;
	}

	if (minHeight > *object->height){
		*object->height = minHeight;
		*object->realHeight = minHeight;
	}

	if (!SetWindowPos(*object->handle, NULL, *object->x, *object->y, *object->width, *object->height, SWP_NOMOVE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;

	if (!InvalidateRect(*object->handle, NULL, FALSE))
		return FALSE;

	return TRUE;
}

/* Sets the minimum size for a GUIObject specified in the thread's context (the self-reference mechanism) to a new value specified by minWidth
   and minHeight */
static BOOL setMinSizeSelfRef(int minWidth, int minHeight){
	return setMinSize((struct _guiobject*)getCurrentThis(), minWidth, minHeight);
}

/* Sets a GUIObject's maximum size to a new value specified by maxWidth and maxHeight */
static BOOL setMaxSize(struct _guiobject *object, int maxWidth, int maxHeight){
	if (!object)
		return FALSE;

	*object->maxWidth = maxWidth;
	*object->maxHeight = maxHeight;

	if (maxWidth < *object->width){
		*object->width = maxWidth;
		*object->realWidth = maxWidth;
	}

	if (maxHeight < *object->height){
		*object->height = maxHeight;
		*object->realHeight = maxHeight;
	}

	if (!SetWindowPos(*object->handle, NULL, *object->x, *object->y, *object->width, *object->height, SWP_NOMOVE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;

	if (!InvalidateRect(*object->handle, NULL, FALSE))
		return FALSE;

	return TRUE;
}

/* Sets the maximum size for a GUIObject specified in the thread's context (the self-reference mechanism) to a new value specified by maxWidth
   and maxHeight */
static BOOL setMaxSizeSelfRef(int maxWidth, int maxHeight){
	return setMaxSize((struct _guiobject*)getCurrentThis(), maxWidth, maxHeight);
}

/* Sets a new text for a GUIObject */
static BOOL setText(struct _guiobject *object, char *text){
	int textLength = strlen(text);
	char *tempReallocPointer;
	
	if (!object)
		return FALSE;

	tempReallocPointer = (char*)realloc(*object->text, textLength + 1);

	if (!tempReallocPointer)
		return FALSE;
	strcpy_s(tempReallocPointer, textLength + 1, text);
	*object->text = tempReallocPointer;
	if (*object->handle)
		return SetWindowTextA(*object->handle, text);
	else
		return TRUE;
}

/* Sets a new text for a GUIObject specified in the thread's context (the self-reference mechanism) */
static BOOL setTextSelfRef(char *text){
	return setText((struct _guiobject*)getCurrentThis(), text);
}

/* Sets a GUIObject's enabled state */
static BOOL setEnabled(struct _guiobject *object, BOOL enabled){
	if (!object)
		return FALSE;
	*object->enabled = enabled;

	if (*object->handle)
		EnableWindow(*object->handle, enabled);
	return TRUE;
}

/* Sets the enabled state for a GUIObject specified in the thread's context (the self-reference mechanism) */
static BOOL setEnabledSelfRef(BOOL enabled){
	return setEnabled((struct _guiobject*)getCurrentThis(), enabled);
}

/* Draws a line in a GUIObject from the point specified by x1, y1 to the point specified by x2, y2 */
static BOOL drawLine(struct _guiobject *object, struct _pen *pen, int x1, int y1, int x2, int y2){
	HPEN prevPen = NULL;
	POINT prevPoint;
	if (!object || !pen)
		return FALSE;

	prevPen = (HPEN)SelectObject(*object->paintContext, *pen->handle);
	if (!prevPen || !MoveToEx(*object->paintContext, x1, y1, &prevPoint))
		return FALSE;
	if (!LineTo(*object->paintContext, x2, y2))
		return FALSE;
	/* Restore previous values */
	if (!MoveToEx(*object->paintContext, prevPoint.x, prevPoint.y, &prevPoint) || !SelectObject(*object->paintContext, prevPen))
		return FALSE;

	return TRUE;
}

/* Draws a line in a GUIObject specified in the thread's context (the self-reference mechanism) from the point specified by
   x1, y1 to the point specified by x2, y2 */
static BOOL drawLineSelfRef(struct _pen *pen, int x1, int y1, int x2, int y2){
	return drawLine((struct _guiobject*)getCurrentThis(), pen, x1, y1, x2, y2);
}

/* Draws an arc in a GUIObject */
static BOOL drawArc(struct _guiobject *object, struct _pen *pen, int boundX1, int boundY1, int boundX2, int boundY2, int x1, int y1, int x2, int y2){
	HPEN prevPen = NULL;
	if (!object || !pen)
		return FALSE;

	prevPen = (HPEN)SelectObject(*object->paintContext, *pen->handle);
	if (!prevPen)
		return FALSE;
	if (!Arc(*object->paintContext, boundX1, boundY1, boundX2, boundY2, x1, y1, x2, y2))
		return FALSE;
	/* Restore previous values */
	if (!SelectObject(*object->paintContext, prevPen))
		return FALSE;

	return TRUE;
}

/* Draws an arc in a GUIObject specified in the thread's context (the self-reference mechanism) */
static BOOL drawArcSelfRef(struct _pen *pen, int boundX1, int boundY1, int boundX2, int boundY2, int x1, int y1, int x2, int y2){
	return drawArc((struct _guiobject*)getCurrentThis(), pen, boundX1, boundY1, boundX2, boundY2, x1, y1, x2, y2);
}

/* Draws a rectangle in a GUIObject */
static BOOL drawRect(struct _guiobject *object, struct _pen *pen, struct _brush *brush, int boundX1, int boundY1, int boundX2, int boundY2){
	HPEN prevPen = NULL;
	HBRUSH prevBrush = NULL, activeBrush;
	if (!object || !pen)
		return FALSE;
	if (!brush)
		activeBrush = GetStockBrush(HOLLOW_BRUSH); /* If no brush was passed, use a hollow brush */
	else
		activeBrush = *brush->handle;

	prevPen = (HPEN)SelectObject(*object->paintContext, *pen->handle);
	prevBrush = (HBRUSH)SelectObject(*object->paintContext, activeBrush);
	if (!prevPen || !prevBrush)
		return FALSE;
	if (!Rectangle(*object->paintContext, boundX1, boundY1, boundX2, boundY2))
		return FALSE;
	/* Restore previous values */
	if (!SelectObject(*object->paintContext, prevPen) || !SelectObject(*object->paintContext, prevBrush))
		return FALSE;

	return TRUE;
}

/* Draws a rectangle in a GUIObject specified in the thread's context (the self-reference mechanism) */
static BOOL drawRectSelfRef(struct _pen *pen, struct _brush *brush, int boundX1, int boundY1, int boundX2, int boundY2){
	return drawRect((struct _guiobject*)getCurrentThis(), pen, brush, boundX1, boundY1, boundX2, boundY2);
}

/* Draws a rounded rectangle in a GUIObject */
static BOOL drawRoundedRect(struct _guiobject *object, struct _pen *pen, struct _brush *brush, int boundX1, int boundY1, 
							int boundX2, int boundY2, int ellipseWidth, int ellipseHeight){
	HPEN prevPen = NULL;
	HBRUSH prevBrush = NULL, activeBrush;
	if (!object || !pen)
		return FALSE;
	if (!brush)
		activeBrush = GetStockBrush(HOLLOW_BRUSH); /* If no brush was passed, use a hollow brush */
	else
		activeBrush = *brush->handle;

	prevPen = (HPEN)SelectObject(*object->paintContext, *pen->handle);
	prevBrush = (HBRUSH)SelectObject(*object->paintContext, activeBrush);
	if (!prevPen || !prevBrush)
		return FALSE;
	if (!RoundRect(*object->paintContext, boundX1, boundY1, boundX2, boundY2, ellipseWidth, ellipseHeight))
		return FALSE;
	/* Restore previous values */
	if (!SelectObject(*object->paintContext, prevPen) || !SelectObject(*object->paintContext, prevBrush))
		return FALSE;

	return TRUE;
}

/* Draws a rounded rectangle in a GUIObject specified in the thread's context (the self-reference mechanism) */
static BOOL drawRoundedRectSelfRef(struct _pen *pen, struct _brush *brush, int boundX1, int boundY1, int boundX2, int boundY2, 
								   int ellipseWidth, int ellipseHeight){
	return drawRoundedRect((struct _guiobject*)getCurrentThis(), pen, brush, boundX1, boundY1, boundX2, boundY2, ellipseWidth, ellipseHeight);
}

/* Draws an ellipse in a GUIObject */
static BOOL drawEllipse(struct _guiobject *object, struct _pen *pen, struct _brush *brush, int boundX1, int boundY1, int boundX2, int boundY2){
	HPEN prevPen = NULL;
	HBRUSH prevBrush = NULL, activeBrush;
	if (!object || !pen)
		return FALSE;
	if (!brush)
		activeBrush = GetStockBrush(HOLLOW_BRUSH); /* If no brush was passed, use a hollow brush */
	else
		activeBrush = *brush->handle;

	prevPen = (HPEN)SelectObject(*object->paintContext, *pen->handle);
	prevBrush = (HBRUSH)SelectObject(*object->paintContext, activeBrush);
	if (!prevPen || !prevBrush)
		return FALSE;
	if (!Ellipse(*object->paintContext, boundX1, boundY1, boundX2, boundY2))
		return FALSE;
	/* Restore previous values */
	if (!SelectObject(*object->paintContext, prevPen) || !SelectObject(*object->paintContext, prevBrush))
		return FALSE;

	return TRUE;
}

/* Draws an ellipse in a GUIObject specified in the thread's context (the self-reference mechanism) */
static BOOL drawEllipseSelfRef(struct _pen *pen, struct _brush *brush, int boundX1, int boundY1, int boundX2, int boundY2){
	return drawEllipse((struct _guiobject*)getCurrentThis(), pen, brush, boundX1, boundY1, boundX2, boundY2);
}

/* Draws a polygon in a GUIObject */
static BOOL drawPolygon(struct _guiobject *object, struct _pen *pen, struct _brush *brush, int numPoints, LONG *coords){
	HPEN prevPen = NULL;
	HBRUSH prevBrush = NULL, activeBrush;
	POINT *points = (POINT*)malloc(numPoints * sizeof(POINT));
	int i;

	if (!object || !pen || numPoints < 2)
		return FALSE;
	if (!brush)
		activeBrush = GetStockBrush(HOLLOW_BRUSH); /* If no brush was passed, use a hollow brush */
	else
		activeBrush = *brush->handle;

	for (i = 0; i < numPoints * 2; i+=2){
		points[i / 2].x = coords[i];
		points[i / 2].y = coords[i + 1];
	}

	prevPen = (HPEN)SelectObject(*object->paintContext, *pen->handle);
	prevBrush = (HBRUSH)SelectObject(*object->paintContext, activeBrush);
	if (!prevPen || !prevBrush)
		return FALSE;
	if (!Polygon(*object->paintContext, points, numPoints))
		return FALSE;
	/* Restore previous values */
	if (!SelectObject(*object->paintContext, prevPen) || !SelectObject(*object->paintContext, prevBrush))
		return FALSE;

	return TRUE;
}

/* Draws a polygon in a GUIObject specified in the thread's context (the self-reference mechanism) */
static BOOL drawPolygonSelfRef(struct _pen *pen, struct _brush *brush, int numPoints, LONG *coords){
	return drawPolygon((struct _guiobject*)getCurrentThis(), pen, brush, numPoints, coords);
}


/* -------------------------------------------------The Constructor-------------------------------------------------------------------------------*/
struct _guiobject *newGUIObject(HINSTANCE instance, char *text, int width, int height){
	static HMENU ID = NULL;
	struct _guiobject *object = (struct _guiobject*)malloc(sizeof(struct _guiobject));
	int IDLength, textLength;

	if (!object)
		return NULL;

	inheritFromObject(object, newObject());

	/* Set the fields */

	safeInit(object->origProcPtr, LONG_PTR, NULL);

	safeInit(object->handle, HWND, NULL);
	safeInit(object->moduleInstance, HINSTANCE, instance);

	safeInit(object->paintContext, HDC, NULL);
	object->paintData = (PAINTSTRUCT*)malloc(sizeof(PAINTSTRUCT)); if (!object->paintData) return NULL;

	*object->type = GUIOBJECT;

	EnterCriticalSection(object->criticalSection);

	IDLength = getNumLength((unsigned int)ID);
	object->className = (char**)malloc(sizeof(char*)); 
	if(!object->className){ LeaveCriticalSection(object->criticalSection); return NULL; }
	*object->className = (char*)malloc(IDLength + 1);

	if (!*object->className){ LeaveCriticalSection(object->criticalSection); return NULL; }
	sprintf_s(*object->className, IDLength + 1, "%d", ID);

	object->ID = (HMENU*)malloc(sizeof(HMENU)); if(!object->ID){ LeaveCriticalSection(object->criticalSection); return NULL; } *object->ID = ID;
	ID++;

	LeaveCriticalSection(object->criticalSection);

	safeInit(object->styles, DWORD, 0x00000000);
	safeInit(object->exStyles, DWORD, 0x00000000);
	
	safeInit(object->numEvents, unsigned int, 1);
	safeInit(object->events, struct _event*, malloc(*object->numEvents * sizeof(struct _event))); //TODO: add more events
	if (!*object->events)
		return NULL;
	(*object->events)[0].eventFunction = NULL; (*object->events)[0].mode = SYNC; (*object->events)[0].message = WM_LBUTTONUP;
	(*object->events)[0].sender = object; (*object->events)[0].context = NULL; (*object->events)[0].args = newEventArgs(WM_LBUTTONUP, 0, 0);
	(*object->events)[0].condition = NULL; (*object->events)[0].interrupt = FALSE; (*object->events)[0].enabled = FALSE;

	textLength = strlen(text);
	safeInit(object->text, char*, malloc(textLength + 1));
	if (!*object->text)
		return NULL;
	strcpy_s(*object->text, textLength + 1, text); /* Copy the object text to the heap */

	safeInit(object->width, int, width); safeInit(object->height, int, height);
	safeInit(object->realWidth, int, width); safeInit(object->realHeight, int, height);
	safeInit(object->minWidth, int, 0); safeInit(object->minHeight, int, 0);
	safeInit(object->maxWidth, int, INT_MAX); safeInit(object->maxHeight, int, INT_MAX);

	safeInit(object->x, int, CW_USEDEFAULT); safeInit(object->y, int, CW_USEDEFAULT);
	safeInit(object->realX, int, CW_USEDEFAULT); safeInit(object->realY, int, CW_USEDEFAULT);

	safeInit(object->enabled, BOOL, TRUE);

	safeInit(object->parent, struct _guiobject*, NULL);
	safeInit(object->children, struct _guiobject**, NULL);
	safeInit(object->numChildren, unsigned int, 0);
	
	/* Set the method pointers */
	object->addChild = &addChildSelfRef; object->addChildT = &addChild; 
	object->setEvent = &setEventSelfRef; object->setEventT = &setEvent;
	object->setEventCondition = &setEventConditionSelfRef; object->setEventConditionT = &setEventCondition;
	object->setEventInterrupt = &setEventInterruptSelfRef; object->setEventInterruptT = &setEventInterrupt;
	object->setEventEnabled = &setEventEnabledSelfRef; object->setEventEnabledT = &setEventEnabled;
	object->setOnClick = &setOnClickSelfRef; object->setOnClickT = &setOnClick;
	object->setPos = &setPosSelfRef; object->setPosT = &setPos;
	object->setSize = &setSizeSelfRef; object->setSizeT = &setSize;
	object->setMinSize = &setMinSizeSelfRef; object->setMinSizeT = &setMinSize;
	object->setMaxSize = &setMaxSizeSelfRef; object->setMaxSizeT = &setMaxSize;
	object->setText = &setTextSelfRef; object->setTextT = &setText;
	object->setEnabled = &setEnabledSelfRef; object->setEnabledT = &setEnabled;
	object->drawLine = &drawLineSelfRef; object->drawLineT = &drawLine;
	object->drawArc = &drawArcSelfRef; object->drawArcT = &drawArc;
	//object->drawText = &drawText; object->drawTextT = &drawText; //TODO! Add fonts!
	object->drawRect = &drawRectSelfRef; object->drawRectT = &drawRect;
	object->drawRoundedRect = &drawRoundedRectSelfRef; object->drawRoundedRectT = &drawRoundedRect;
	object->drawEllipse = &drawEllipseSelfRef; object->drawEllipseT = &drawEllipse;
	object->drawPolygon = &drawPolygonSelfRef; object->drawPolygonT = &drawPolygon;

	return object;
}

/* -------------------------------------------------The Destructor--------------------------------------------------------------------------------*/
void deleteGUIObject(struct _guiobject *object){
	unsigned int i;
	EndPaint(*object->handle, object->paintData);
	DestroyWindow(*object->handle);

	free(object->origProcPtr); free(object->handle); free(object->moduleInstance);
	free(object->paintContext); free(object->paintData);
	if (object->className)
		free(*object->className);
	free(object->className);
	free(object->styles); free(object->exStyles); free(object->ID);
	if (object->events){
		for (i = 0; i < *object->numEvents; i++)
			if ((*object->events)[i].args)
				if (*(*object->events)[i].args->type == EVENTARGS)
					deleteEventArgs((*object->events)[i].args);
				else if (*(*object->events)[i].args->type == MOUSEEVENTARGS)
					deleteMouseEventArgs((struct _mouseeventargs*)(*object->events)[i].args);
		free(*object->events);
	}
	free(object->numEvents); free(object->events);
	if (object->text)
		free(*object->text);
	free(object->text); free(object->width); free(object->height); free(object->x); free(object->y); free(object->enabled); free(object->parent);
	free(object->children); free(object->numChildren);

	deleteObject((struct _object*)object);
}





/* ======================================================Class Window============================================================================== */
/* -------------------------------------------------The Methods----------------------------------------------------------------------------------*/
/* Changes a window's resizable style */
static BOOL setResizable(struct _window *window, BOOL resizable){
	*window->resizable = resizable;
	*window->styles = resizable ? (*window->styles) | WS_THICKFRAME : (*window->styles) ^ WS_THICKFRAME;

	if (*window->handle)
		return SetWindowLongPtrA(*window->handle, GWL_STYLE, (LONG)(*window->styles)) ? TRUE : FALSE;
	else
		return TRUE;
}

/* Changes the resizable style for a window specified in the thread's context (the self-reference mechanism). 
   Not thread-safe */
static BOOL setResizableSelfRef(BOOL resizable){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setResizable((struct _window*)currentThisLocal, resizable);
	else
		return FALSE;
}

/* Enables or disables a window's maximize box */
static BOOL enableMaximize(struct _window *window, BOOL maximize){
	if (!window)
		return FALSE;
	*window->maximizeEnabled = maximize;
	*window->styles = maximize ? *window->styles | WS_MAXIMIZEBOX : *window->styles ^ WS_MAXIMIZEBOX;

	if (*window->handle)
		return SetWindowLongPtrA(*window->handle, GWL_STYLE, (LONG)(*window->styles)) ? TRUE : FALSE;
	else
		return TRUE;
}

/* Enables or disables the maximize box for a window specified in the thread's context (the self-reference mechanism). 
   Not thread-safe */
static BOOL enableMaximizeSelfRef(BOOL maximize){
	return enableMaximize((struct _window*)getCurrentThis(), maximize);
}


/* -------------------------------------------------The Constructor-------------------------------------------------------------------------------*/
struct _window *newWindow(HINSTANCE instance, char *text, int width, int height){
	struct _window *window = (struct _window*)malloc(sizeof(struct _window));

	if (!window)
		return NULL;

	inheritFromGUIObject(window, newGUIObject(instance, text, width, height));

	/* Set the fields */
	*window->type = WINDOW;
	*window->styles = WS_OVERLAPPEDWINDOW;
	*window->exStyles = WS_EX_WINDOWEDGE;
	
	safeInit(window->clientWidth, int, 0);
	safeInit(window->clientHeight, int, 0);

	safeInit(window->resizable, BOOL, TRUE);
	safeInit(window->maximizeEnabled, BOOL, TRUE);

	/* Set the method pointers */
	window->setResizable = &setResizableSelfRef; window->setResizableT = &setResizable;
	window->enableMaximize = &enableMaximizeSelfRef; window->enableMaximizeT = &enableMaximize;

	return window;
}

/* -------------------------------------------------The Destructor--------------------------------------------------------------------------------*/
void deleteWindow(struct _window *window){
	free(window->clientWidth); free(window->clientHeight); free(window->resizable); free(window->maximizeEnabled);
	deleteGUIObject((struct _guiobject*)window);
}





/* ======================================================Class Control============================================================================= */
/* -------------------------------------------------The Methods----------------------------------------------------------------------------------*/
/* Moves a GUIObject to a new location specified by x and y */
/* Overrides setPosT in GUIObject */
static BOOL _control_setPos(struct _guiobject *object, int x, int y){
	if (!object)
		return FALSE;
	
	*object->realX = x;
	*object->realY = y;

	if (x >= *((struct _control*)object)->minX && x <= *((struct _control*)object)->maxX)
		*object->x = x;
	if (y >= *((struct _control*)object)->minY && y <= *((struct _control*)object)->maxY)
		*object->y = y;

	if (!SetWindowPos(*object->handle, NULL, *object->x, *object->y, *object->width, *object->height, SWP_NOSIZE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;

	if (!InvalidateRect(*object->handle, NULL, FALSE))
		return FALSE;

	return TRUE;
}

/* Moves a GUIObject specified in the thread's context (the self-reference mechanism) to a new location specified by x
   and y */
/* Overrides setPos in GUIObject */
static BOOL _control_setPosSelfRef(int x, int y){
	return _control_setPos((struct _guiobject*)getCurrentThis(), x, y);
}

/* Sets a control's minimum position to a new value specified by minX and minY */
static BOOL setMinPos(struct _control* object, int minX, int minY){
	if (!object)
		return FALSE;

	*object->minX = minX;
	*object->minY = minY;

	if (minX > *object->x){
		*object->x = minX;
		*object->realX = minX;
	}

	if (minY > *object->y){
		*object->y = minY;
		*object->realY = minY;
	}

	if (!SetWindowPos(*object->handle, NULL, *object->x, *object->y, *object->width, *object->height, SWP_NOMOVE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;

	if (!InvalidateRect(*object->handle, NULL, FALSE))
		return FALSE;

	return TRUE;
}

/* Sets the minimum position for a control specified in the thread's context (the self-reference mechanism) to a new value specified by minX
   and minY */
static BOOL setMinPosSelfRef(int minWidth, int minHeight){
	return setMinPos((struct _control*)getCurrentThis(), minWidth, minHeight);
}

/* Sets a control's maximum position to a new value specified by maxX and maxY */
static BOOL setMaxPos(struct _control* object, int maxX, int maxY){
	if (!object)
		return FALSE;

	*object->maxX = maxX;
	*object->maxY = maxY;

	if (maxX < *object->x){
		*object->x = maxX;
		*object->realX = maxX;
	}

	if (maxY < *object->y){
		*object->y = maxY;
		*object->realY = maxY;
	}

	if (!SetWindowPos(*object->handle, NULL, *object->x, *object->y, *object->width, *object->height, SWP_NOMOVE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;

	if (!InvalidateRect(*object->handle, NULL, FALSE))
		return FALSE;

	return TRUE;
}

/* Sets the maximum position for a control specified in the thread's context (the self-reference mechanism) to a new value specified by maxX
   and maxY */
static BOOL setMaxPosSelfRef(int maxWidth, int maxHeight){
	return setMaxPos((struct _control*)getCurrentThis(), maxWidth, maxHeight);
}


/* -------------------------------------------------The Constructor------------------------------------------------------------------------------*/
struct _control *newControl(HINSTANCE instance, char *text, int width, int height, int x, int y){
	struct _control *control = (struct _control*)malloc(sizeof(struct _control));

	if (!control)
		return NULL;

	inheritFromGUIObject(control, newGUIObject(instance, text, width, height));
	
	safeInit(control->minX, int, INT_MIN); safeInit(control->minY, int, INT_MIN);
	safeInit(control->maxX, int, INT_MAX); safeInit(control->maxY, int, INT_MAX);
	safeInit(control->anchor, short, 0xFF00);

	*control->type = CONTROL;
	*control->x = x; *control->realX = x;
	*control->y = y; *control->realY = y;
	*control->exStyles = WS_EX_WINDOWEDGE;

	/* Override virtual methods */
	control->setPos = &_control_setPosSelfRef;
	control->setPosT = &_control_setPos;
	control->base.setPos = &_control_setPosSelfRef;
	control->base.setPosT = &_control_setPos;
	

	control->setMinPos = &setMinPosSelfRef; control->setMinPosT = &setMinPos;
	control->setMaxPos = &setMaxPosSelfRef; control->setMaxPosT = &setMaxPos;

	return control;
}

/* -------------------------------------------------The Destructor--------------------------------------------------------------------------------*/
void deleteControl(struct _control *control){
	deleteGUIObject((struct _guiobject*)control);
}





/* ======================================================Class Button============================================================================== */
/* -------------------------------------------------The Constructor------------------------------------------------------------------------------*/
struct _button *newButton(HINSTANCE instance, char *text, int width, int height, int x, int y){
	struct _button *button = (struct _button*)malloc(sizeof(struct _button));

	if (!button)
		return NULL;
	inheritFromControl(button, newControl(instance, text, width, height, x, y));

	*button->type = BUTTON;
	free(*button->className);
	*button->className = "Button";
	*button->styles = WS_CHILD | WS_VISIBLE | BS_TEXT | BS_PUSHBUTTON;

	return button;
}

/* -------------------------------------------------The Destructor--------------------------------------------------------------------------------*/
void deleteButton(struct _button *button){
	*button->className = NULL;
	deleteControl((struct _control*)button);
}





/* ======================================================Class TextBox============================================================================== */
/* -------------------------------------------------The Methods----------------------------------------------------------------------------------*/
/* Sets the text input mode for a textbox to number-only or to not number-only */
static BOOL setNumOnly(struct _textbox* textbox, BOOL numOnly){
	*textbox->numOnly = numOnly;
	*textbox->styles = numOnly ? *textbox->styles | ES_NUMBER : *textbox->styles ^ ES_NUMBER;

	if (*textbox->handle)
		return SetWindowLongPtrA(*textbox->handle, GWL_STYLE, (LONG)*textbox->styles) ? TRUE : FALSE;
	else
		return TRUE;
}

/* Sets the text input mode for a textbox to number-only for a textbox specified in the thread's context (the self-reference mechanism). 
   Not thread-safe */
static BOOL setNumOnlySelfRef(BOOL numOnly){
	return setNumOnly((struct _textbox*)getCurrentThis(), numOnly);
}



/* -------------------------------------------------The Constructor------------------------------------------------------------------------------*/
struct _textbox *newTextBox(HINSTANCE instance, char *text, int width, int height, int x, int y, enum _textboxtype multiline){
	struct _textbox *textbox = (struct _textbox*)malloc(sizeof(struct _textbox));

	if (!textbox)
		return NULL;

	inheritFromControl(textbox, newControl(instance, text, width, height, x, y));

	*textbox->type = TEXTBOX;
	free(*textbox->className);
	*textbox->className = "Edit";
	*textbox->styles = WS_CHILD | WS_VISIBLE | WS_BORDER;

	safeInit(textbox->multiline, BOOL, multiline);
	if (*textbox->multiline)
		*textbox->styles |= ES_MULTILINE | ES_WANTRETURN;

	safeInit(textbox->numOnly, BOOL, FALSE);

	textbox->setNumOnly = &setNumOnlySelfRef; textbox->setNumOnlyT = &setNumOnly;

	return textbox;
}

/* -------------------------------------------------The Destructor--------------------------------------------------------------------------------*/
void deleteTextBox(struct _textbox *textbox){
	*textbox->className = NULL;
	deleteControl((struct _control*)textbox);
}





/* ======================================================Class Label=============================================================================== */
/* -------------------------------------------------The Constructor------------------------------------------------------------------------------*/
struct _label *newLabel(HINSTANCE instance, char *text, int width, int height, int x, int y){
	struct _label *label = (struct _label*)malloc(sizeof(struct _label));

	if (!label)
		return NULL;

	inheritFromControl(label, newControl(instance, text, width, height, x, y));

	*label->type = LABEL;
	free(*label->className);
	*label->className = "Static"; //TODO: Place in heap, for destructor!
	*label->styles = WS_CHILD | WS_VISIBLE | SS_NOTIFY;

	return label;
}

/* -------------------------------------------------The Destructor--------------------------------------------------------------------------------*/
void deleteLabel(struct _label *label){
	*label->className = NULL;
	deleteControl((struct _control*)label);
}





/* ======================================================Class EventArgs=========================================================================== */
/* -------------------------------------------------The Methods----------------------------------------------------------------------------------*/
/* Updates the value of an EventArgs object */
static BOOL updateValue(struct _eventargs *eventargs, UINT message, WPARAM wParam, LPARAM lParam){
	if (!eventargs)
		return FALSE;

	*eventargs->message = message;
	*eventargs->wParam = wParam;
	*eventargs->lParam = lParam;

	return TRUE;
}

/* Updates the value of an EventArgs object specified in the thread's context (the self-reference mechanism) */
static BOOL updateValueSelfRef(UINT message, WPARAM wParam, LPARAM lParam){
	return updateValue((struct _eventargs*)getCurrentThis(), message, wParam, lParam);
}


/* -------------------------------------------------The Constructor------------------------------------------------------------------------------*/
struct _eventargs *newEventArgs(UINT message, WPARAM wParam, LPARAM lParam){
	struct _eventargs *eventargs = (struct _eventargs*)malloc(sizeof(struct _eventargs));

	if (!eventargs)
		return NULL;

	inheritFromObject(eventargs, newObject());

	*eventargs->type = EVENTARGS;

	safeInit(eventargs->message, UINT, message);
	safeInit(eventargs->wParam, WPARAM, wParam);
	safeInit(eventargs->lParam, LPARAM, lParam);

	eventargs->updateValue = &updateValueSelfRef;
	eventargs->updateValueT = &updateValue;

	return eventargs;
}

/* -------------------------------------------------The Destructor--------------------------------------------------------------------------------*/
void deleteEventArgs(struct _eventargs *eventargs){
	free(eventargs->message);
	free(eventargs->wParam);
	free(eventargs->lParam);

	deleteObject((struct _object*)eventargs);
}





/* ======================================================Class MouseEventArgs====================================================================== */
/* -------------------------------------------------The Methods----------------------------------------------------------------------------------*/
/* Updates the value of a MouseEventArgs object */
/* Overrides updateValueT in EventArgs */
static BOOL _mouseeventargs_updateValue(struct _eventargs *eventargs, UINT message, WPARAM wParam, LPARAM lParam){
	if (!eventargs)
		return FALSE;

	*eventargs->message = message;
	*eventargs->wParam = wParam;
	*eventargs->lParam = lParam;
	*((struct _mouseeventargs*)eventargs)->cursorX = GET_X_LPARAM(lParam);
	*((struct _mouseeventargs*)eventargs)->cursorY = GET_Y_LPARAM(lParam);

	return TRUE;
}

/* Updates the value of a MouseEventArgs object specified in the thread's context (the self-reference mechanism) */
/* Overrides updateValue in EventArgs */
static BOOL _mouseeventargs_updateValueSelfRef(UINT message, WPARAM wParam, LPARAM lParam){
	return _mouseeventargs_updateValue((struct _eventargs*)getCurrentThis(), message, wParam, lParam);
}


/* -------------------------------------------------The Constructor------------------------------------------------------------------------------*/
struct _mouseeventargs *newMouseEventArgs(UINT message, WPARAM wParam, LPARAM lParam){
	struct _mouseeventargs *mouseeventargs = (struct _mouseeventargs*)malloc(sizeof(struct _mouseeventargs));
	if (!mouseeventargs)
		return NULL;

	inheritFromEventArgs(mouseeventargs, newEventArgs(message, wParam, lParam));

	safeInit(mouseeventargs->cursorX, int, GET_X_LPARAM(lParam));
	safeInit(mouseeventargs->cursorY, int, GET_Y_LPARAM(lParam));

	*mouseeventargs->type = MOUSEEVENTARGS;

	/* Override virtual methods */
	mouseeventargs->updateValue = &_mouseeventargs_updateValueSelfRef;
	mouseeventargs->updateValueT = &_mouseeventargs_updateValue;
	mouseeventargs->base.updateValue = &_mouseeventargs_updateValueSelfRef;
	mouseeventargs->base.updateValueT = &_mouseeventargs_updateValue;

	return mouseeventargs;
}

/* -------------------------------------------------The Destructor--------------------------------------------------------------------------------*/
void deleteMouseEventArgs(struct _mouseeventargs *mouseeventargs){
	free(mouseeventargs->cursorX);
	free(mouseeventargs->cursorY);
	deleteEventArgs((struct _eventargs*)mouseeventargs);
}





/* ======================================================Class Pen================================================================================= */
/* -------------------------------------------------The Methods----------------------------------------------------------------------------------*/
/* Updates the value of a Pen object */
static BOOL _pen_updateValue(struct _pen *pen, int penStyle, int width, COLORREF color){
	if (!pen)
		return FALSE;
	*pen->penStyle = penStyle;
	*pen->width = width;
	*pen->color = color;
	if (!DeleteObject(*pen->handle))
		return FALSE;
	*pen->handle = CreatePen(penStyle, width, color);
	if (!*pen->handle)
		return FALSE;
	return TRUE;
}

/* Updates the value of a Pen object specified in the thread's context (the self-reference mechanism) */
static BOOL _pen_updateValueSelfRef(int penStyle, int width, COLORREF color){
	return _pen_updateValue((struct _pen*)getCurrentThis(), penStyle, width, color);
}

/* -------------------------------------------------The Constructor------------------------------------------------------------------------------*/
struct _pen *newPen(int penStyle, int width, COLORREF color){
	struct _pen *pen = (struct _pen*)malloc(sizeof(struct _pen));

	if (!pen)
		return NULL;

	inheritFromObject(pen, newObject());

	*pen->type = PEN;

	safeInit(pen->penStyle, int, penStyle);
	safeInit(pen->width, int, width);
	safeInit(pen->color, COLORREF, color);
	safeInit(pen->handle, HPEN, CreatePen(penStyle, width, color));

	pen->updateValue = &_pen_updateValueSelfRef; pen->updateValueT = &_pen_updateValue;

	return pen;
}

/* -------------------------------------------------The Destructor--------------------------------------------------------------------------------*/
void deletePen(struct _pen *pen){
	free(pen->penStyle); free(pen->width); free(pen->color); DeleteObject(*pen->handle); free(pen->handle);
	deleteObject((struct _object*)pen);
}





/* ======================================================Class Brush=============================================================================== */
/* -------------------------------------------------The Methods----------------------------------------------------------------------------------*/
/* Updates the value of a Brush object */
static BOOL _brush_updateValue(struct _brush *brush, UINT brushStyle, COLORREF color, ULONG_PTR hatch){
	LOGBRUSH brushInfo;
	if (!brush)
		return FALSE;

	*brush->brushStyle = brushStyle; brushInfo.lbStyle = brushStyle;
	*brush->color = color; brushInfo.lbColor = color;
	*brush->hatch = hatch; brushInfo.lbHatch = hatch;
	if (!DeleteObject(*brush->handle))
		return FALSE;
	*brush->handle = CreateBrushIndirect(&brushInfo);
	if (!*brush->handle)
		return FALSE;
	return TRUE;
}

/* Updates the value of a Brush object specified in the thread's context (the self-reference mechanism) */
static BOOL _brush_updateValueSelfRef(UINT brushStyle, COLORREF color, ULONG_PTR hatch){
	return _brush_updateValue((struct _brush*)getCurrentThis(), brushStyle, color, hatch);
}

/* -------------------------------------------------The Constructor------------------------------------------------------------------------------*/
struct _brush *newBrush(UINT brushStyle, COLORREF color, ULONG_PTR hatch){
	struct _brush *brush = (struct _brush*)malloc(sizeof(struct _brush));
	LOGBRUSH brushInfo;

	if (!brush)
		return NULL;

	inheritFromObject(brush, newObject());

	*brush->type = BRUSH;

	safeInit(brush->brushStyle, UINT, brushStyle); brushInfo.lbStyle = brushStyle;
	safeInit(brush->color, COLORREF, color); brushInfo.lbColor = color;
	safeInit(brush->hatch, ULONG_PTR, hatch); brushInfo.lbHatch = hatch;
	safeInit(brush->handle, HBRUSH, CreateBrushIndirect(&brushInfo));

	brush->updateValue = &_brush_updateValueSelfRef; brush->updateValueT = &_brush_updateValue;

	return brush;
}

/* -------------------------------------------------The Destructor--------------------------------------------------------------------------------*/
void deleteBrush(struct _brush *brush){
	free(brush->brushStyle); free(brush->color); DeleteObject(*brush->handle); free(brush->handle);
	deleteObject((struct _object*)brush);
}





/* =============================================WINAPI CALL FUNCTIONS=============================================================================== */

/*---------------------------------------------EVENT HANDLING------------------------------------------------------------*/
/* The common asynchronous event callback function */
static DWORD WINAPI asyncEventProc(LPVOID event){
	struct _event *eventPointer = (struct _event*)event;
	eventPointer->eventFunction(eventPointer->sender, eventPointer->context, eventPointer->args);
	return TRUE;
}

/* Find and fire off an event for a GUIObject */
static int handleEvents(struct _guiobject *currObject, UINT messageID, WPARAM wParam, LPARAM lParam){
	unsigned int i;
	BOOL condition = TRUE;

	for (i = 0; i < *currObject->numEvents; i++){
		if  ((*currObject->events)[i].message == messageID && (*currObject->events)[i].eventFunction != NULL){
			if ((*currObject->events)[i].condition)
				condition = *((*currObject->events)[i].condition);

			if ((*currObject->events)[i].enabled && condition && (*currObject->events)[i].eventFunction){
				EnterCriticalSection((*currObject->events)[i].args->criticalSection);
				(*currObject->events)[i].args->updateValueT((*currObject->events)[i].args, messageID, wParam, lParam); /* Update the event args */
				LeaveCriticalSection((*currObject->events)[i].args->criticalSection);

				if ((*currObject->events)[i].mode == SYNC){
					((*currObject->events)[i].eventFunction)((*currObject->events)[i].sender,
															(*currObject->events)[i].context, (*currObject->events)[i].args);
				} else
					CreateThread(NULL, 0, asyncEventProc, (LPVOID)&(*currObject->events)[i], 0, NULL);
			}

			return (int)i;
		}
	}

	return -1;
}

/* The event handling routine for WM_COMMAND type events for window controls */
static int commandEventHandler(HWND hwnd, WPARAM wParam, LPARAM lParam){
	UINT itemID = LOWORD(wParam), messageID = HIWORD(wParam);
	struct _guiobject *currObject = (struct _guiobject*)GetWindowLongPtrA((HWND)lParam, GWLP_USERDATA);
	char *controlText = NULL;
	int textLength = 0;
	
	if (currObject){
		if (messageID == EN_CHANGE){ /* Text in a textbox was changed by the user */
			textLength = GetWindowTextLengthA(*currObject->handle);
			if (textLength > 0 && (controlText = (char*)malloc(textLength + 1))){
				if (GetWindowTextA(*currObject->handle, controlText, textLength + 1) > 0){
					free(*currObject->text);
					*currObject->text = controlText;
				}
			}
		}

		return handleEvents(currObject, messageID, wParam, lParam);
	} else
		return -1;
}

/*---------------------------------------------FIELD VALUES CONSISTENCY SUPPORT ON REFRESH--------------------------------*/
/* Resizes and/or moves the children of a window or a control according to their anchor settings */
BOOL alignChildren(struct _guiobject *object, int widthChange, int heightChange){
	unsigned int i;
	struct _control *currChild;
	int controlWidthChange = 0, controlHeightChange = 0;

	if (!object)
		return FALSE;

	for (i = 0; i < *object->numChildren; i++){
		if ((*object->children)[i] != NULL){
			currChild = (struct _control*)(*object->children)[i];

			if (((*currChild->anchor & 0xF000) && (*currChild->anchor & 0x000F)) ||
					((*currChild->anchor & 0x0F00) && (*currChild->anchor & 0x00F0))){ /* Anchored top and bottom and/or left and right */
				if ((*currChild->anchor & 0xF000) && (*currChild->anchor & 0x000F)) /* Anchored left and right */
					controlWidthChange = widthChange;

				if ((*currChild->anchor & 0x0F00) && (*currChild->anchor & 0x00F0)) /* Anchored top and bottom */
					controlHeightChange = heightChange;

				EnterCriticalSection(currChild->criticalSection);
				setSize((*object->children)[i], *currChild->realWidth + controlWidthChange, *currChild->realHeight + controlHeightChange);
				LeaveCriticalSection(currChild->criticalSection);

				if (*currChild->numChildren != 0)
					if (!alignChildren((*object->children)[i], controlWidthChange, controlHeightChange))
						return FALSE;
			} 
			
			if ((!(*currChild->anchor & 0xF000) && (*currChild->anchor & 0x000F)) || 
						(!(*currChild->anchor & 0x0F00) && (*currChild->anchor & 0x00F0))){ /* Anchored right but not left and/or bottom but not top */
				EnterCriticalSection(currChild->criticalSection);
				_control_setPos((*object->children)[i], *currChild->realX + widthChange, *currChild->realY + heightChange);
				LeaveCriticalSection(currChild->criticalSection); 
			}

			/* Not anchored left or right */
			if (!(*currChild->anchor & 0xF000) && !(*currChild->anchor & 0x000F)){
				EnterCriticalSection(currChild->criticalSection); EnterCriticalSection((*currChild->parent)->criticalSection);
				setPos((*object->children)[i], *currChild->realX + *(*currChild->parent)->width / 2 - (*(*currChild->parent)->width - widthChange) / 2, 
							*currChild->realY);
				LeaveCriticalSection(currChild->criticalSection); LeaveCriticalSection((*currChild->parent)->criticalSection);
			}

			/* Not anchored top or bottom */
			if  (!(*currChild->anchor & 0x0F00) && !(*currChild->anchor & 0x00F0)) {
				EnterCriticalSection(currChild->criticalSection); EnterCriticalSection((*currChild->parent)->criticalSection);
				setPos((*object->children)[i], *currChild->realX, 
							*currChild->realY + *(*currChild->parent)->height / 2 - (*(*currChild->parent)->height - heightChange) / 2);
				LeaveCriticalSection(currChild->criticalSection); LeaveCriticalSection((*currChild->parent)->criticalSection);
			}
		}
	}

	return TRUE;
}

/* Sets the current window's width, height and clientWidth and clientHeight on user resize. Thread-safe */
static void refreshWindowSize(struct _window *window, LPARAM lParam){
	RECT clientSize;
	WINDOWPOS *windowPos;
	int widthChange = 0, heightChange = 0;

	EnterCriticalSection(window->criticalSection);

	windowPos = (WINDOWPOS*)lParam;
	*window->width = windowPos->cx;
	*window->height = windowPos->cy;
	*window->x = windowPos->x;
	*window->y = windowPos->y;

	if (GetClientRect(*window->handle, &clientSize)){
		widthChange = clientSize.right - clientSize.left - *window->clientWidth;
		heightChange = clientSize.bottom - clientSize.top - *window->clientHeight;

		*window->clientWidth = clientSize.right - clientSize.left;
		*window->clientHeight = clientSize.bottom - clientSize.top;
	}

	LeaveCriticalSection(window->criticalSection);

	alignChildren((struct _guiobject*)window, widthChange, heightChange);
}

/* The window proc prototype */
static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Display a control on a window */
BOOL displayControl(struct _control *control){
	HFONT hFont;
	LOGFONT lf;

	/* Add the control */
	*control->handle = CreateWindowExA(*control->exStyles, *control->className, *control->text, *control->styles, *control->x, *control->y,
		*control->width, *control->height, (*control->parent) ? *(*control->parent)->handle : NULL, *control->ID, *control->moduleInstance, NULL);

	if (!*control->handle)
		return FALSE;

	/* Change its font */
	GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf); 
	hFont = CreateFont(0, 0, 0, 0, 400, 0, 0, lf.lfStrikeOut, lf.lfCharSet, lf.lfOutPrecision, 
							lf.lfClipPrecision, lf.lfQuality, lf.lfPitchAndFamily, lf.lfFaceName);
	//TODO: make the font a property of the Object type!
	SendMessageA(*control->handle, WM_SETFONT, (WPARAM)hFont, TRUE);
		
	/* Set the control handle's additional data to a pointer to its object */
	SetWindowLongPtrA(*control->handle, GWLP_USERDATA, (LONG)control);

	/* Subclass the control to make it send its messages through the main window proc */
	*control->origProcPtr = SetWindowLongPtrA(*control->handle, GWLP_WNDPROC, (LONG_PTR)windowProc);
	if (!*control->origProcPtr)
		return FALSE;
	
	EnableWindow(*control->handle, *control->enabled);

	return TRUE;
}

/* Go through a GUIObject's list of children and add all of them to the window, then call itself on each of the children */
static void displayChildren(struct _guiobject *object){
	unsigned int i;

	if (object)
		for (i = 0; i < *object->numChildren; i++)
			if ((*object->children)[i] != NULL)
				displayControl((struct _control*)(*object->children)[i]);
}


/* The Window Procedure callback function */
static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	struct _guiobject *currObject = NULL;
	int eventID = -1;
	BOOL interrupt = FALSE;

	if (hwnd != NULL)
		currObject = (struct _guiobject*)GetWindowLongPtrA(hwnd, GWLP_USERDATA); /* Get the object that this handle belongs to */

	/* Default event handling */
	switch(msg){
		case WM_CREATE:
			if (currObject)
				displayChildren(currObject);
			break;

		case WM_PAINT:
			if (currObject) /* Begin or end painting the object */
				if (!*currObject->paintContext)
					*currObject->paintContext = BeginPaint(*currObject->handle, currObject->paintData);
			break;

		case WM_COMMAND:
			eventID = commandEventHandler(hwnd, wParam, lParam);
			break;

		case WM_NOTIFY:
			//TODO: add "notify" event handling!
			break;

		case WM_GETMINMAXINFO: /* The window size or position limits are queried */
			if (currObject){
				((MINMAXINFO*)lParam)->ptMaxSize.x = *currObject->maxWidth;
				((MINMAXINFO*)lParam)->ptMaxSize.y = *currObject->maxHeight;
				((MINMAXINFO*)lParam)->ptMaxTrackSize.x = *currObject->maxWidth;
				((MINMAXINFO*)lParam)->ptMaxTrackSize.y = *currObject->maxHeight;
				((MINMAXINFO*)lParam)->ptMinTrackSize.x = *currObject->minWidth;
				((MINMAXINFO*)lParam)->ptMinTrackSize.y = *currObject->minHeight;
			}
			break;

		case WM_WINDOWPOSCHANGED: /* The window size or position have just been changed */
			if (*currObject->type == WINDOW)
				refreshWindowSize((struct _window*)currObject, lParam);
			break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
			break;

        case WM_DESTROY:
            PostQuitMessage(0);
			break;
    }

	if (currObject){
		eventID = handleEvents(currObject, msg, wParam, lParam);
		
		if (msg == WM_PAINT && *currObject->handle) {
			EndPaint(*currObject->handle, currObject->paintData);
			*currObject->paintContext = NULL;
		}
	}
	
	/* Call the window's events */
	if (currObject){
		if (eventID >= 0 && (UINT)eventID < *currObject->numEvents)
			interrupt = (*currObject->events)[eventID].interrupt;

		if (!interrupt){
			if (*currObject->type == WINDOW)
				return DefWindowProcA(hwnd, msg, wParam, lParam);
			else /* Resend the messages to the subclassed object's default window proc */
				return CallWindowProcA((WNDPROC)(*currObject->origProcPtr), hwnd, msg, wParam, lParam);
		}

		return 0;
	} else
		return DefWindowProcA(hwnd, msg, wParam, lParam);
}

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

/* Display a window with the application's command line settings */
BOOL displayWindow(struct _window *mainWindow, int nCmdShow){
    MSG msg;
	RECT clientRect;

	/* Register the window class */
    if (!registerClass(*mainWindow->moduleInstance, *mainWindow->className, (WNDPROC)windowProc))
        return FALSE;

	*mainWindow->handle = CreateWindowExA(*mainWindow->exStyles, *mainWindow->className,
																 *mainWindow->text,
														    	 *mainWindow->styles,
														    	 *mainWindow->x, *mainWindow->y, 
														    	 *mainWindow->width,
														    	 *mainWindow->height,
														    	 NULL, NULL, 
														    	 *mainWindow->moduleInstance, NULL);

	if (!*mainWindow->handle)
		return FALSE;

	/* Set the window handle's additional data to a pointer to its object */
	SetWindowLongPtrA(*mainWindow->handle, GWLP_USERDATA, (LONG)(mainWindow));

	if (GetClientRect(*mainWindow->handle, &clientRect)){
		*mainWindow->clientWidth = clientRect.right - clientRect.left;
		*mainWindow->clientHeight = clientRect.bottom - clientRect.top;
	}

	displayChildren((struct _guiobject*)mainWindow);

	ShowWindow(*mainWindow->handle, nCmdShow);
    UpdateWindow(*mainWindow->handle);
	EnableWindow(*mainWindow->handle, *mainWindow->enabled);

    /* The message loop */
    while(GetMessageA(&msg, NULL, 0, 0) > 0){
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return msg.wParam;
}