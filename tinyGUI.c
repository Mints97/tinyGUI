#include "tinyGUI.h"

#define checkedDereference(pointer) if (!pointer) return NULL; *pointer

/* =============================================THE INHERITANCE MECHANISM================================================== */

/* Sets the pointers of an instance of a type derived from GUIObject to be the same as the ones of its base type value */
#define inheritFromGUIObject(object) object->base->derived = (void*)object; \
	/* "Inherit" from base type */ \
	object->criticalSection = object->base->criticalSection; object->origProcPtr = object->base->origProcPtr; \
	object->handle = object->base->handle; object->moduleInstance = object->base->moduleInstance; \
	object->className = object->base->className; \
	object->ID = object->base->ID; object->styles = object->base->styles; object->exStyles = object->base->exStyles; \
	object->type = object->base->type; \
	/* event handlers */ object->events = object->base->events; object->numEvents = object->base->numEvents; \
	object->setEvent = object->base->setEvent; object->setEventT = object->base->setEventT; \
	/* properties */ object->text = object->base->text; \
	object->width = object->base->width; object->height = object->base->height; \
	object->x = object->base->x; object->y = object->base->y; object->enabled = object->base->enabled; \
	/* links */ object->parent = object->base->parent; object->children = object->base->children; \
	object->numChildren = object->base->numChildren; \
	/* methods */ object->setPos = object->base->setPos; object->setPosT = object->base->setPosT; \
	object->setSize = object->base->setSize; object->setSizeT = object->base->setSizeT; \
	object->setText = object->base->setText; object->setTextT = object->base->setTextT; \
	object->setEnabled = object->base->setEnabled; object->setEnabledT = object->base->setEnabledT;

/* Sets the pointers of an instance inherited from the Control type to be the same as the ones of its base type value */
#define inheritFromControl(object)	object->base->derived = (void*)object; \
	/* "Inherit" from base type */ \
	object->criticalSection = object->base->criticalSection; object->origProcPtr = object->base->origProcPtr; \
	object->handle = object->base->handle; object->moduleInstance = object->base->moduleInstance; \
	object->className = object->base->className; \
	object->ID = object->base->ID; object->styles = object->base->styles; object->exStyles = object->base->exStyles; \
	object->type = object->base->type; \
	/* event handlers */ object->events = object->base->events; object->numEvents = object->base->numEvents; \
	object->setEvent = object->base->setEvent; object->setEventT = object->base->setEventT; \
	/* properties */ object->text = object->base->text; \
	object->width = object->base->width; object->height = object->base->height; \
	object->x = object->base->x; object->y = object->base->y; object->enabled = object->base->enabled; \
	/* links */ object->parent = object->base->parent; object->children = object->base->children; \
	object->numChildren = object->base->numChildren; \
	/* methods */ object->addChild = object->base->addChild; object->addChildT = object->base->addChildT; \
	object->addButton = object->base->addButton; object->addButtonT = object->base->addButtonT; \
	object->setPos = object->base->setPos; object->setPosT = object->base->setPosT; \
	object->setSize = object->base->setSize; object->setSizeT = object->base->setSizeT; \
	object->setText = object->base->setText; object->setTextT = object->base->setTextT; \
	object->setEnabled = object->base->setEnabled; object->setEnabledT = object->base->setEnabledT;


/* =====================================================Thread synchronization================================================================= */
/* Get ownership of a mutex. Used in thread synchronization for accessing shared global context */
static HANDLE getMutexOwnership(char *mutexName){
	HANDLE mutex = NULL;
	DWORD mutexWaitResult;
	mutex = CreateMutexA(NULL, FALSE, mutexName); /* Create named mutex or get a handle to it if it exists */
	
	if (mutex == NULL)
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


/* =============================================THE OBJECT METHODS========================================================= */

/*---------------------------------------------GUIObject class--------------------------------------------------------------*/
/* Moves an object to a new location specified by x and y. Not thread-safe */
static BOOL setPos(struct _guiobject* object, int x, int y){
	*object->x = x;
	*object->y = y;
	if (!SetWindowPos(*object->handle, NULL, x, y, *object->width, *object->height, SWP_NOSIZE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;

	return TRUE;
}

/* Moves an object specified in the thread's context (the self-reference mechanism) to a new location specified by x
   and y. Not thread-safe */
static BOOL setObjectPosSelfRef(int x, int y){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setPos((struct _guiobject*)currentThisLocal, x, y);
	else
		return FALSE;
}

/* Resizes an object to a new size specified by width and height. Not thread-safe */
static BOOL setSize(struct _guiobject* object, int width, int height){*object->width = width;
	*object->height = height;
	if (!SetWindowPos(*object->handle, NULL, *object->x, *object->y, width, height, SWP_NOMOVE | SWP_ASYNCWINDOWPOS | 
																					SWP_DRAWFRAME))
		return FALSE;

	return TRUE;
}

/* Resizes an object specified in the thread's context (the self-reference mechanism) to a new size specified by width
   and height. Not thread-safe */
static BOOL setObjectSizeSelfRef(int width, int height){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setSize((struct _guiobject*)currentThisLocal, width, height);
	else
		return FALSE;
}

/* Sets an event for an object by a Windows message. Not thread-safe */
static BOOL setEvent(struct _guiobject *object, DWORD message, void(*callback)(void*, void*, struct _eventArgs*), void *context, enum _syncMode mode){
	struct _event *tempReallocPointer;
	unsigned int i;
	void *sender;

	switch(*object->type){
		case WINDOW:
			sender = object->derived;
			break;
		case CONTROL:
			sender = object->derived;
			break;
		case BUTTON:
			sender = ((struct _control*)object->derived) != NULL ? ((struct _control*)object->derived)->derived : NULL;
			break;
		default:
			sender = (void*)object;
	}

	for (i = 0; i < *object->numEvents; i++)
		if  ((*object->events)[i].message == message){
			(*object->events)[i].eventFunction = callback;
			(*object->events)[i].sender = sender;
			(*object->events)[i].context = context;
			(*object->events)[i].mode = mode;
			return TRUE;
		}

	(*object->numEvents)++;
	tempReallocPointer = (struct _event*)realloc(*object->events, *object->numEvents * sizeof(struct _event));
	if (!tempReallocPointer){
		return FALSE;
	}

	*object->events = tempReallocPointer;
	/* Problem: why can't I access the newly allocated events as object->event[i]->field? */
	(*object->events)[*object->numEvents - 1].message = message;
	(*object->events)[*object->numEvents - 1].eventFunction = callback;
	(*object->events)[*object->numEvents - 1].sender = sender;
	(*object->events)[*object->numEvents - 1].context = context;
	(*object->events)[*object->numEvents - 1].mode = mode;

	return TRUE;
}

/* Sets an event for an object specified in the thread's context (the self-reference mechanism) on a Windows message. Not thread-safe */
static BOOL setEventSelfRef(DWORD message, void(*callback)(void*, void*, struct _eventArgs*), void *context, enum _syncMode mode){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEvent((struct _guiobject*)currentThisLocal, message, callback, context, mode);
	else
		return FALSE;
}

/* Sets a new text for an object. Not thread-safe */
static BOOL setText(struct _guiobject *object, char *text){
	int textLength = strlen(text);
	char *tempReallocPointer = (char*)realloc(*object->text, textLength + 1);
	if (!tempReallocPointer)
		return FALSE;
	strcpy_s(tempReallocPointer, textLength + 1, text);
	*object->text = tempReallocPointer;
	if (*object->handle)
		return SetWindowTextA(*object->handle, text);
	else
		return TRUE;
}

/* Sets a new text for an object specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setTextSelfRef(char *text){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setText((struct _guiobject*)currentThisLocal, text);
	else
		return FALSE;
}

/* Sets an object's enabled state. Not thread-safe */
static BOOL setEnabled(struct _guiobject *object, BOOL enabled){
	return EnableWindow(*object->handle, enabled);
}

/* Sets the enabled state for an object specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setEnabledSelfRef(BOOL enabled){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEnabled((struct _guiobject*)currentThisLocal, enabled);
	else
		return FALSE;
}


/*---------------------------------------------Window class--------------------------------------------------------------*/
/* Adds a control to a window (self-reference mechanism not used). Not thread-safe */
static BOOL addChildToWindow(struct _window *thisWindow, struct _control *child){
	struct _guiobject **newChildrenPointer;

	*child->parent = thisWindow->base;
	(*thisWindow->numChildren)++;
	newChildrenPointer = (struct _guiobject**)realloc(*thisWindow->children, *thisWindow->numChildren * sizeof(struct _guiobject*));

	if (newChildrenPointer == NULL)
		return FALSE;

	*thisWindow->children = newChildrenPointer;
	(*thisWindow->children)[*thisWindow->numChildren - 1] = child->base;

	return TRUE;
}

/* Adds a control to the current window specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL addChildToWindowSelfRef(struct _control *child){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return addChildToWindow((struct _window*)currentThisLocal, child);
	else
		return FALSE;
}

/* Moves a window specified in the thread's context (the self-reference mechanism) to a new location specified by x 
   and y. Not thread-safe */
static BOOL setWindowPosSelfRef(int x, int y){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setPos(((struct _window*)currentThisLocal)->base, x, y);
	else
		return FALSE;
}

/* Resizes a window specified in the thread's context (the self-reference mechanism) to a new size specified by width
   and height. Not thread-safe */
static BOOL setWindowSizeSelfRef(int width, int height){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setSize(((struct _window*)currentThisLocal)->base, width, height);
	else
		return FALSE;
}

/* Sets an event for a window specified in the thread's context (the self-reference mechanism) on a Windows message. Not thread-safe */
static BOOL setEventForWindowSelfRef(DWORD message, void(*callback)(void*, void*, struct _eventArgs*), void *context, enum _syncMode mode){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEvent(((struct _window*)currentThisLocal)->base, message, callback, context, mode);
	else
		return FALSE;
}

/* Sets a WM_LBUTTONUP event for a window. Not thread-safe */
static BOOL setOnClickForWindow(struct _window *object, void(*callback)(struct _window*, void*, struct _eventArgs*), void *context, enum _syncMode mode){
	return setEvent(object->base, WM_LBUTTONUP, (void(*)(void*, void*, struct _eventArgs*))callback, context, mode);
}

/* Sets a WM_LBUTTONUP event for a window specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setOnClickForWindowSelfRef(void(*callback)(struct _window*, void*, struct _eventArgs*), void *context, enum _syncMode mode){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEvent(((struct _window*)currentThisLocal)->base, WM_LBUTTONUP, (void(*)(void*, void*, struct _eventArgs*))callback,
																						context, mode);
	else
		return FALSE;
}

/* Changes a window's resizable style. Not thread-safe */
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

/* Enables or disables a window's maximize box. Not thread-safe */
static BOOL enableMaximize(struct _window *window, BOOL maximize){
	if (window){
		*window->maximizeEnabled = maximize;
		*window->styles = maximize ? *window->styles | WS_MAXIMIZEBOX : *window->styles ^ WS_MAXIMIZEBOX;

		if (*window->handle)
			return SetWindowLongPtrA(*window->handle, GWL_STYLE, (LONG)(*window->styles)) ? TRUE : FALSE;
		else
			return TRUE;
	} else
		return FALSE;
}

/* Enables or disables the maximize box for a window specified in the thread's context (the self-reference mechanism). 
   Not thread-safe */
static BOOL enableMaximizeSelfRef(BOOL maximize){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return enableMaximize((struct _window*)currentThisLocal, maximize);
	else
		return FALSE;
}

/* Sets a new text for a window specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setWindowTextSelfRef(char *text){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setText(((struct _window*)currentThisLocal)->base, text);
	else
		return FALSE;
}

/* Sets the enabled state for a window specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setWindowEnabledSelfRef(BOOL enabled){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEnabled(((struct _window*)currentThisLocal)->base, enabled);
	else
		return FALSE;
}


/*---------------------------------------------Control class--------------------------------------------------------*/
/* Adds a control to a window (self-reference mechanism not used). Not thread-safe */
static BOOL addChildToControl(struct _control *thisControl, struct _control *child){
	struct _guiobject **newChildrenPointer;

	*child->parent = thisControl->base;
	(*thisControl->numChildren)++;
	newChildrenPointer = (struct _guiobject**)realloc(*thisControl->children, 
														*thisControl->numChildren * sizeof(struct _guiobject*));

	if (newChildrenPointer == NULL)
		return FALSE;

	*thisControl->children = newChildrenPointer;
	(*thisControl->children)[*thisControl->numChildren - 1] = child->base;

	return TRUE;
}

/* Adds a control to the current control specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL addChildToControlSelfRef(struct _control *child){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return addChildToControl((struct _control*)currentThisLocal, child);
	else
		return FALSE;
}

/* Adds a button to a window (self-reference mechanism not used). Not thread-safe */
static BOOL addButtonToControl(struct _control *thisControl, struct _button *child){
	return addChildToControl(thisControl, child->base);
}

/* Adds a button to the current window specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL addButtonToControlSelfRef(struct _button *child){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return addButtonToControl((struct _control*)currentThisLocal, child);
	else
		return FALSE;
}

/* Moves a control specified in the thread's context (the self-reference mechanism) to a new location specified by x
   and y. Not thread-safe */
static BOOL setControlPosSelfRef(int x, int y){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setPos(((struct _control*)currentThisLocal)->base, x, y);
	else
		return FALSE;
}

/* Resizes a control specified in the thread's context (the self-reference mechanism) to a new size specified by width
   and height. Not thread-safe */
static BOOL setControlSizeSelfRef(int width, int height){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setSize(((struct _control*)currentThisLocal)->base, width, height);
	else
		return FALSE;
}

/* Sets an event for a control specified in the thread's context (the self-reference mechanism) on a Windows message. Not thread-safe */
static BOOL setEventForControlSelfRef(DWORD message, void(*callback)(void*, void*, struct _eventArgs*), void *context, enum _syncMode mode){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEvent(((struct _control*)currentThisLocal)->base, message, callback, context, mode);
	else
		return FALSE;
}

/* Sets a new text for a control specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setControlTextSelfRef(char *text){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setText(((struct _control*)currentThisLocal)->base, text);
	else
		return FALSE;
}

/* Sets the enabled state for a control specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setControlEnabledSelfRef(BOOL enabled){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEnabled(((struct _control*)currentThisLocal)->base, enabled);
	else
		return FALSE;
}


/*---------------------------------------------Button class---------------------------------------------------------*/
/* Moves a button specified in the thread's context (the self-reference mechanism) to a new location specified by x 
   and y. Not thread-safe */
static BOOL setButtonPosSelfRef(int x, int y){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setPos(((struct _button*)currentThisLocal)->base->base, x, y);
	else
		return FALSE;
}

/* Resizes a button specified in the thread's context (the self-reference mechanism) to a new size specified by width
   and height. Not thread-safe */
static BOOL setButtonSizeSelfRef(int width, int height){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setSize(((struct _button*)currentThisLocal)->base->base, width, height);
	else
		return FALSE;
}

/* Sets an event for a button specified in the thread's context (the self-reference mechanism) on a Windows message. Not thread-safe */
static BOOL setEventForButtonSelfRef(DWORD message, void(*callback)(void*, void*, struct _eventArgs*), void *context, enum _syncMode mode){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEvent(((struct _button*)currentThisLocal)->base->base, message, callback, context, mode);
	else
		return FALSE;
}

/* Sets a BN_CLICKED event for a button. Not thread-safe */
static BOOL setOnClickForButton(struct _button *object, void(*callback)(struct _button*, void*, struct _eventArgs*), void *context, enum _syncMode mode){
	return setEvent(object->base->base, BN_CLICKED, (void(*)(void*, void*, struct _eventArgs*))callback, context, mode);
}

/* Sets a BN_CLICKED event for a button specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setOnClickForButtonSelfRef(void(*callback)(struct _button*, void*, struct _eventArgs*), void *context, enum _syncMode mode){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEvent(((struct _button*)currentThisLocal)->base->base, BN_CLICKED, (void(*)(void*, void*, struct _eventArgs*))callback, context, mode);
	else
		return FALSE;
}

/* Sets a new text for a window specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setButtonTextSelfRef(char *text){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setText(((struct _button*)currentThisLocal)->base->base, text);
	else
		return FALSE;
}

/* Sets the enabled state for a button specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setButtonEnabledSelfRef(BOOL enabled){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEnabled(((struct _button*)currentThisLocal)->base->base, enabled);
	else
		return FALSE;
}


/*---------------------------------------------TextBox class---------------------------------------------------------*/
/* Sets the text input mode for a textbox to number-only. Not thread-safe */
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
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setNumOnly((struct _textbox*)currentThisLocal, numOnly);
	else
		return FALSE;
}

/* Sets a new text for a window specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setTextBoxTextSelfRef(char *text){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setText(((struct _textbox*)currentThisLocal)->base->base, text);
	else
		return FALSE;
}

/* Sets the enabled state for a textbox specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setTextBoxEnabledSelfRef(BOOL enabled){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEnabled(((struct _textbox*)currentThisLocal)->base->base, enabled);
	else
		return FALSE;
}


/*---------------------------------------------Label class---------------------------------------------------------*/
/* Sets a new text for a window specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setLabelTextSelfRef(char *text){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setText(((struct _label*)currentThisLocal)->base->base, text);
	else
		return FALSE;
}

/* Sets the enabled state for a label specified in the thread's context (the self-reference mechanism). Not thread-safe */
static BOOL setLabelEnabledSelfRef(BOOL enabled){
	void *currentThisLocal = getCurrentThis();
	if (currentThisLocal != NULL)
		return setEnabled(((struct _label*)currentThisLocal)->base->base, enabled);
	else
		return FALSE;
}


/* ============================================OTHER======================================================================= */
/* Quickly get the number of symbols in the decimal representation of a number. */
static int getNumLength(unsigned int x) {
    if(x>=1000000000) return 10; if(x>=100000000) return 9; if(x>=10000000) return 8; if(x>=1000000) return 7;
    if(x>=100000) return 6; if(x>=10000) return 5; if(x>=1000) return 4; if(x>=100) return 3; if(x>=10) return 2;
    return 1;
}


/* =============================================THE OBJECT CONSTRUCTORS==================================================== */

/* Initialize a new instance of the Object type with a unique identifier. Thread-safe */
struct _guiobject *newObject(HINSTANCE instance, char *text, int width, int height){
	static HMENU ID = NULL;
	struct _guiobject *object = (struct _guiobject*)malloc(sizeof(struct _guiobject));
	int IDLength, textLength;

	if (!object)
		return NULL;

	object->derived = NULL;

	object->criticalSection = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
	if (object->criticalSection) InitializeCriticalSection(object->criticalSection); else return NULL;

	object->origProcPtr = (LONG_PTR*)malloc(sizeof(LONG_PTR)); checkedDereference(object->origProcPtr) = (LONG_PTR)NULL;

	object->handle = (HWND*)malloc(sizeof(HWND)); checkedDereference(object->handle) = NULL;
	object->moduleInstance = (HINSTANCE*)malloc(sizeof(HINSTANCE)); checkedDereference(object->moduleInstance) = instance;
	object->type = (enum _objectType*)malloc(sizeof(enum _objectType)); checkedDereference(object->type) = OBJECT;

	EnterCriticalSection(object->criticalSection);

	IDLength = getNumLength((unsigned int)ID);
	object->className = (char**)malloc(sizeof(char*)); checkedDereference(object->className) = (char*)malloc(IDLength + 1);
	if (!*object->className){
		LeaveCriticalSection(object->criticalSection);
		return NULL;
	}
	sprintf_s(*object->className, IDLength + 1, "%d", ID);

	object->ID = (HMENU*)malloc(sizeof(HMENU)); if(!object->ID){ LeaveCriticalSection(object->criticalSection); return NULL; } *object->ID = ID;
	ID++;

	LeaveCriticalSection(object->criticalSection);

	object->styles = (DWORD*)malloc(sizeof(DWORD)); checkedDereference(object->styles) = 0x00000000;
	object->exStyles = (DWORD*)malloc(sizeof(DWORD)); checkedDereference(object->exStyles) = 0x00000000;
	
	object->numEvents = (unsigned int*)malloc(sizeof(unsigned int)); checkedDereference(object->numEvents) = 1;
	object->events = (struct _event**)malloc(sizeof(struct _event*));
	checkedDereference(object->events) = (struct _event*)malloc(*object->numEvents * sizeof(struct _event)); //TODO: add more events
	if (!*object->events)
		return NULL;
	object->events[0]->eventFunction = NULL; object->events[0]->mode = SYNC; object->events[0]->message = 0x00000000;
												object->events[0]->sender = NULL; object->events[0]->context = NULL;

	object->setEvent = &setEventSelfRef;
	object->setEventT = &setEvent;

	textLength = strlen(text);
	object->text = (char**)malloc(sizeof(char*)); checkedDereference(object->text) = (char*)malloc(textLength + 1);
	if (!*object->text)
		return NULL;
	strcpy_s(*object->text, textLength + 1, text); /* Copy the object text to the heap */

	object->width = (int*)malloc(sizeof(int)); checkedDereference(object->width) = width;
	object->height = (int*)malloc(sizeof(int)); checkedDereference(object->height) = height;

	object->x = (int*)malloc(sizeof(int)); checkedDereference(object->x) = CW_USEDEFAULT;
	object->y = (int*)malloc(sizeof(int)); checkedDereference(object->y) = CW_USEDEFAULT;

	object->enabled = (BOOL*)malloc(sizeof(BOOL)); checkedDereference(object->enabled) = TRUE;

	object->parent = (struct _guiobject**)malloc(sizeof(struct _guiobject*)); checkedDereference(object->parent) = NULL;
	object->children = (struct _guiobject***)malloc(sizeof(struct _guiobject**)); checkedDereference(object->children) = NULL;
	object->numChildren = (unsigned int*)malloc(sizeof(unsigned int)); checkedDereference(object->numChildren) = 0;

	object->setPos = &setObjectPosSelfRef;
	object->setPosT = &setPos;

	object->setSize = &setObjectSizeSelfRef;
	object->setSizeT = &setSize;

	object->setText = &setTextSelfRef;
	object->setTextT = &setText;

	object->setEnabled = &setEnabledSelfRef;
	object->setEnabledT = &setEnabled;

	return object;
}

/* Initialize a new instance of the Window type */
struct _window *newWindow(HINSTANCE instance, char *text, int width, int height){
	struct _window *window = (struct _window*)malloc(sizeof(struct _window));

	if (window == NULL)
		return NULL;

	window->base = newObject(instance, text, width, height);
	if (!window->base)
		return NULL;
	inheritFromGUIObject(window);
	window->derived = NULL;

	*window->type = WINDOW;
	*window->styles = WS_OVERLAPPEDWINDOW;
	*window->exStyles = WS_EX_WINDOWEDGE;
	
	window->clientWidth = (int*)malloc(sizeof(int)); checkedDereference(window->clientWidth) = 0;
	window->clientHeight = (int*)malloc(sizeof(int)); checkedDereference(window->clientHeight) = 0;

	window->resizable = (BOOL*)malloc(sizeof(BOOL)); checkedDereference(window->resizable) = TRUE;
	window->maximizeEnabled = (BOOL*)malloc(sizeof(BOOL)); checkedDereference(window->maximizeEnabled) = TRUE;

	window->addChildT = &addChildToWindow;
	window->addChild = &addChildToWindowSelfRef;

	window->setEvent = &setEventForWindowSelfRef;

	window->setOnClick = &setOnClickForWindowSelfRef;
	window->setOnClickT = &setOnClickForWindow;

	window->setPos = &setWindowPosSelfRef;
	window->setSize = &setWindowSizeSelfRef;

	window->setResizable = &setResizableSelfRef;
	window->setResizableT = &setResizable;

	window->enableMaximize = &enableMaximizeSelfRef;
	window->enableMaximizeT = &enableMaximize;

	window->setText = &setWindowTextSelfRef;

	window->setEnabled = &setWindowEnabledSelfRef;

	return window;
}

/* Initialize a new instance of the Control type */
struct _control *newControl(HINSTANCE instance, char *text, int width, int height, int x, int y){
	struct _control *control = (struct _control*)malloc(sizeof(struct _control));

	if (control == NULL)
		return NULL;

	control->base = newObject(instance, text, width, height);
	if (control->base == NULL)
		return NULL;
	inheritFromGUIObject(control);
	control->derived = NULL;

	*control->type = CONTROL;
	*control->x = x;
	*control->y = y;
	*control->exStyles = WS_EX_WINDOWEDGE;

	control->addChildT = &addChildToControl;
	control->addChild = &addChildToControlSelfRef;
	control->addButtonT = &addButtonToControl;
	control->addButton = &addButtonToControlSelfRef;

	control->setEvent = &setEventForControlSelfRef;

	control->setPos = &setControlPosSelfRef;
	control->setSize = &setControlSizeSelfRef;

	control->setText = &setControlTextSelfRef;

	control->setEnabled = &setControlEnabledSelfRef;

	return control;
}

/* Initialize a new instance of the Button type */
struct _button *newButton(HINSTANCE instance, char *text, int width, int height, int x, int y){
	struct _button *button = (struct _button*)malloc(sizeof(struct _button));

	if (button == NULL)
		return NULL;

	button->base = newControl(instance, text, width, height, x, y);
	if (button->base == NULL)
		return NULL;
	inheritFromControl(button);
	//button->derived = NULL;

	*button->type = BUTTON;
	free(*button->className);
	*button->className = "Button"; //TODO: Place in heap, for destructor!
	*button->styles = WS_CHILD | WS_VISIBLE | BS_TEXT | BS_PUSHBUTTON;

	button->setPos = &setButtonPosSelfRef;
	button->setSize = &setButtonSizeSelfRef;

	button->setEvent = &setEventForButtonSelfRef;

	button->setOnClick = &setOnClickForButtonSelfRef;
	button->setOnClickT = &setOnClickForButton;

	button->setText = &setButtonTextSelfRef;

	button->setEnabled = &setButtonEnabledSelfRef;

	return button;
}

/* Initialize a new instance of the TextBox type */
struct _textbox *newTextBox(HINSTANCE instance, char *text, int width, int height, int x, int y, enum _textboxtype multiline){
	struct _textbox *textbox = (struct _textbox*)malloc(sizeof(struct _textbox));

	if (textbox == NULL)
		return NULL;

	textbox->base = newControl(instance, text, width, height, x, y);
	if (textbox->base == NULL)
		return NULL;
	inheritFromControl(textbox);
	//textbox->derived = NULL;

	*textbox->type = TEXTBOX;
	free(*textbox->className);
	*textbox->className = "Edit"; //TODO: Place in heap, for destructor!
	*textbox->styles = WS_CHILD | WS_VISIBLE | WS_BORDER;

	textbox->multiline = (BOOL*)malloc(sizeof(BOOL)); checkedDereference(textbox->multiline) = multiline;
	if (*textbox->multiline)
		*textbox->styles |= ES_MULTILINE | ES_WANTRETURN;

	textbox->numOnly = (BOOL*)malloc(sizeof(BOOL)); checkedDereference(textbox->numOnly) = FALSE;

	//TODO!
	//textbox->setPos = &setTextBoxPosSelfRef;
	//textbox->setSize = &setTextBoxSizeSelfRef;

	//textbox->setEvent = &setEventForTextBoxSelfRef;

	//textbox->setOnClick = &setOnClickForTextBoxSelfRef;
	//textbox->setOnClickT = &setOnClickForTextBox;

	textbox->setNumOnly = &setNumOnlySelfRef;
	textbox->setNumOnlyT = &setNumOnly;

	textbox->setText = &setTextBoxTextSelfRef;

	textbox->setEnabled = &setTextBoxEnabledSelfRef;

	return textbox;
}

struct _label *newLabel(HINSTANCE instance, char *text, int width, int height, int x, int y){
	struct _label *label = (struct _label*)malloc(sizeof(struct _label));

	if (label == NULL)
		return NULL;

	label->base = newControl(instance, text, width, height, x, y);
	if (label->base == NULL)
		return NULL;
	inheritFromControl(label);
	//label->derived = NULL;

	*label->type = LABEL;
	free(*label->className);
	*label->className = "Static"; //TODO: Place in heap, for destructor!
	*label->styles = WS_CHILD | WS_VISIBLE;

	//TODO!
	//label->setPos = &setLabelPosSelfRef;
	//label->setSize = &setLabelSizeSelfRef;

	//label->setEvent = &setEventForLabelSelfRef;

	//label->setOnClick = &setOnClickForLabelSelfRef;
	//label->setOnClickT = &setOnClickForLabel;

	label->setText = &setLabelTextSelfRef;

	label->setEnabled = &setLabelEnabledSelfRef;

	return label;
}


/* =============================================THE OBJECT DESTRUCTORS==================================================== */

BOOL deleteObject(struct _guiobject *object){
	if (!object)
		return TRUE;

	if (object->derived) 
		return FALSE; /* A destructor cannot destroy a base type value of an object! (maybe add this functionality later?) */

	if (object->criticalSection) DeleteCriticalSection(object->criticalSection);
	free(object->criticalSection);

	free(object->origProcPtr); free(object->handle); free(object->moduleInstance); free(object->type);
	if (object->className)
		free(*object->className);
	free(object->className);
	free(object->styles); free(object->exStyles); free(object->ID); free(object->numEvents);
	if (object->events)
		free(*object->events);
	free(object->events);
	if (object->text)
		free(*object->text);
	free(object->text); free(object->width); free(object->height); free(object->x); free(object->y); free(object->enabled); free(object->parent);
	free(object->children); free(object->numChildren);
	return TRUE;
}

BOOL deleteWindow(struct _window *window){
	if (!deleteObject(window->base))
		return FALSE;
	free(window->clientWidth); free(window->clientHeight); free(window->resizable); free(window->maximizeEnabled);
	return TRUE;
}

BOOL deleteControl(struct _control *control){
	return deleteObject(control->base);
}

BOOL deleteButton(struct _button *button){
	*button->className = NULL;
	return deleteControl(button->base);
}

BOOL deleteTextBox(struct _textbox *textbox){
	*textbox->className = NULL;
	return deleteControl(textbox->base);
}

BOOL deleteLabel(struct _label *label){
	*label->className = NULL;
	return deleteControl(label->base);
}


/* =============================================WINAPI CALL FUNCTIONS====================================================== */

/*---------------------------------------------EVENT HANDLING------------------------------------------------------------*/
/* The common asynchronous event callback function */
static DWORD WINAPI asyncEventProc(LPVOID event){
	struct _event *eventPointer = (struct _event*)event;
	eventPointer->eventFunction(eventPointer->sender, eventPointer->context, NULL); //TODO: add event arg sending!
	return TRUE;
}

/* Find and fire off an event for an object */
static void handleEvents(struct _guiobject *currObject, UINT messageID){
	unsigned int i;

	for (i = 0; i < *currObject->numEvents; i++){
		if  ((*currObject->events)[i].message == messageID && (*currObject->events)[i].eventFunction != NULL){
			if ((*currObject->events)[i].mode != ASYNC)
				((*currObject->events)[i].eventFunction)((*currObject->events)[i].sender,
														(*currObject->events)[i].context, NULL); //TODO: add event arg sending!
			else
				CreateThread(NULL, 0, asyncEventProc, (LPVOID)&(*currObject->events)[i], 0, NULL);
		}
	}
}

/* The event handling routine for WM_COMMAND type events for window controls */
static void commandEventHandler(HWND hwnd, WPARAM wParam, LPARAM lParam){
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

		handleEvents(currObject, messageID);
	}
}

/*---------------------------------------------FIELD VALUES CONSISTENCY SUPPORT ON REFRESH--------------------------------*/
/* Sets the current window's width, height and clientWidth and clientHeight on user resize. Thread-safe */
static void refreshWindowSize(struct _window *window, LPARAM lParam){
	RECT clientSize;
	WINDOWPOS *windowPos;

	EnterCriticalSection(window->criticalSection);

	windowPos = (WINDOWPOS*)lParam;
	*window->width = windowPos->cx;
	*window->height = windowPos->cy;
	*window->x = windowPos->x;
	*window->y = windowPos->y;

	if (GetClientRect(*window->handle, &clientSize)){
		*window->clientWidth = clientSize.right - clientSize.left;
		*window->clientHeight = clientSize.bottom - clientSize.top;
	}

	LeaveCriticalSection(window->criticalSection);
}


/* The Window Procedure callback function */
static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	struct _guiobject *currObject = NULL;
	struct _window *window = NULL;

	if (hwnd != NULL){
		currObject = (struct _guiobject*)GetWindowLongPtrA(hwnd, GWLP_USERDATA); /* Get the object that this handle belongs to */
		if (currObject && *currObject->type == WINDOW) /* If the window has been created */
			window = (struct _window*)(currObject->derived);
	}

	if (currObject)
		handleEvents(currObject, msg);

	switch(msg){
		case WM_CREATE:
			//Move start call to child window adding to here?
			break;

		case WM_COMMAND:
			commandEventHandler(hwnd, wParam, lParam);
			break;

		case WM_NOTIFY:
			//TODO: add "notify" event handling!
			break;

		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
			//TODO: add children resizing!
			if (window)
				refreshWindowSize(window, lParam);
			break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
			break;

        case WM_DESTROY:
            PostQuitMessage(0);
			break;
    }
	
	/* Call the window's events */
	if (currObject){
		if (window)
			return DefWindowProcA(hwnd, msg, wParam, lParam);
		else /* Resend the messages to the subclassed object's default window proc. Note: make option to intercept these on event handling! */
			return CallWindowProcA((WNDPROC)(*currObject->origProcPtr), hwnd, msg, wParam, lParam);
	}

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

/* Display a control on a window */
static HWND showControl(HWND hwnd, HINSTANCE hInstance, HMENU controlID, char *controlType, DWORD controlStyle, DWORD controlExStyle,
						char *controlText, int controlX, int controlY, int controlWidth, int controlHeight){
	HWND hControl;
	HFONT hFont;
	LOGFONT lf;

	/* Add the control */
	hControl = CreateWindowExA(controlExStyle, controlType, controlText, controlStyle, controlX, controlY, controlWidth,
								controlHeight, hwnd, controlID, hInstance, NULL);

	/* Change its font */
	GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf); 
	hFont = CreateFont(0, 0, 0, 0, 400, 0, 0, lf.lfStrikeOut, lf.lfCharSet, lf.lfOutPrecision, 
							lf.lfClipPrecision, lf.lfQuality, lf.lfPitchAndFamily, lf.lfFaceName);
	//TODO: make the font a property of the Object type!

	SendMessageA(hControl, WM_SETFONT, (WPARAM)hFont, TRUE);

	return hControl;
}

/* Go through an object's list of children and add all of them to the window, then call itself on each of the children */
static void addChildren(struct _guiobject *control){
	unsigned int i;

	for (i = 0; i < *control->numChildren; i++){
		*((*control->children)[i])->handle = showControl(*control->handle, *control->moduleInstance, *((*control->children)[i])->ID,
													   *((*control->children)[i])->className, *((*control->children)[i])->styles,
													   *((*control->children)[i])->exStyles,
													   *((*control->children)[i])->text, 
													   *((*control->children)[i])->x, *((*control->children)[i])->y,
													   *((*control->children)[i])->width,
													   *((*control->children)[i])->height);
		

		/* Subclass the control to make it send its messages through the main window proc */
		*((*control->children)[i])->origProcPtr = SetWindowLongA(*((*control->children)[i])->handle, GWLP_WNDPROC, (LONG_PTR)windowProc);
		
		/* Set the control handle's additional data to a pointer to its object */
		SetWindowLongPtrA(*((*control->children)[i])->handle, GWLP_USERDATA, (LONG)((*control->children)[i]));
		
		EnableWindow(*((*control->children)[i])->handle, *((*control->children)[i])->enabled);

		if (*((*control->children)[i])->numChildren != 0)
			addChildren((*control->children)[i]);
	}
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
	SetWindowLongPtrA(*mainWindow->handle, GWLP_USERDATA, (LONG)(mainWindow->base));

	if (GetClientRect(*mainWindow->handle, &clientRect)){
		*mainWindow->clientWidth = clientRect.right - clientRect.left;
		*mainWindow->clientHeight = clientRect.bottom - clientRect.top;
	}

	addChildren(mainWindow->base);

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
